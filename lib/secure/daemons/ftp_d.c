/* Do not remove the headers from this file! see /USAGE for more info. */

/* 
** First draft / quickly hacked FTPD.
**
** Rust (rust@lima.mudlib.org) July 12, 1996
**
** There are commands this doesn't support.  If your client
** Seems to want some other command to work for some weird reason,
** and this FTPD won't do it, let me know and I'll add it in.
**
** Myth@Icon of Sin - Jan 19, 1997
**  o Fixed STOR:
**    o ascii: removed \r's
**    o binary: added a fileposition flag
**  o Added the SYST command.
**  o Added line checking to the read callback.
**  o Fixed send so that larger files can be handled without difficulty.
** Jan 21, 1997
**  o Fixed the mkd command.
**
*/

#include <socket.h>
#include <assert.h>
#include <log.h>
#include <clean_up.h>
#include <ftp_d.h>
#include <ports.h>

inherit M_ACCESS;

private static mapping sessions = ([]);
private static mapping dataports = ([]);
private static mapping dispatch = 
([
  "user" : (: FTP_CMD_user :),
  "pass" : (: FTP_CMD_pass :),
  "retr" : (: FTP_CMD_retr :),
  "stor" : (: FTP_CMD_stor :),
  "nlst" : (: FTP_CMD_nlst :),
  "list" : (: FTP_CMD_list :),
  "pwd"  : (: FTP_CMD_pwd  :),
  "cdup" : (: FTP_CMD_cdup :),
  "cwd"  : (: FTP_CMD_cwd  :),
  "quit" : (: FTP_CMD_quit :),
  "type" : (: FTP_CMD_type :),
  "mkd"  : (: FTP_CMD_mkd  :),
  "port" : (: FTP_CMD_port :),
  "noop" : (: FTP_CMD_noop :),
  "dele" : (: FTP_CMD_dele :),
  "syst" : (: FTP_CMD_syst :),
]);

private object sock;

private mixed outfile=([]);

#ifdef ALLOW_ANON_FTP
private static string array anon_logins = ({"anonymous", "ftp"});
#endif

private void FTPLOGF(string format, string array args...)
{
  FTPLOG(sprintf(format, args...));
}

private string FTP_get_welcome_msg()
{
  return sprintf("220- %s FTP server ready.\n%s"
		 "220 Anonymous and wizard logins accepted.\n",
		 mud_name(),
		 is_file(FTP_WELCOME) ? "220- " +
		 replace_string(read_file(FTP_WELCOME), "\n", "\n220- ") 
		 +"\n": "");
}

private void FTP_handle_idlers()
{
  map_delete(sessions,0); /* Just make sure there's no stale stuff here */
  if(!sizeof(sessions))
    {
      return;
    }

  foreach(object socket, class ftp_session info in sessions)
    {
      if(info->dataPipe) /* Data connections are still active. */
	{
	  info->idleTime = 0;
	  continue;
	}
      info->idleTime += 60;
      if(info->idleTime > MAX_IDLE_TIME+60)
	{
	  FTPLOGF("%s idled out at %s.\n", info->user, ctime(time()));
	  destruct(info->cmdPipe);
	}
    }
  call_out("FTP_handle_idlers", 60);
}

private void FTP_addSession(object socket)
{
  class ftp_session  newSession;

  newSession = new(class ftp_session);
  newSession->cmdPipe = socket;
#ifdef ALLOW_ANON_FTP
  socket->send(FTP_get_welcome_msg());
#else
  socket->send(sprintf("220- %s FTP server ready.\n220 Please login with your"
	       " wizard name.\n", mud_name()));
#endif
  map_delete(sessions,0); /* Just make sure there's no stale stuff here */
  if(!sizeof(sessions))
    {
      call_out("FTP_handle_idlers", 60);
    }
  sessions[socket] = newSession;
}

private void FTP_write(object socket)
{
  class ftp_session	info;

  info = sessions[dataports[socket]];
  info->cmdPipe->send("226 Transfer complete.\n");
  info->dataPipe->remove();
}

private void FTP_read(object socket, string data)
{
  string                cmd, arg;
  class ftp_session	thisSession;
  function		dispatchTo;
  int i;

  /* If there is no data, it's a new connection. */
  if (!data)
    {
      assert(!sessions[socket]);
      FTP_addSession(socket);
      return;
    }

  thisSession = sessions[socket];
  thisSession->idleTime = 0;

  if (!thisSession->command) thisSession->command="";

  data=replace_string(data,"\r","");
  thisSession->command+=data;
  if ((i=strsrch(thisSession->command,"\n"))==-1) return;
  data=thisSession->command[0..i-1];
  thisSession->command=thisSession->command[i+1..];

  thisSession->command=trim_spaces(thisSession->command);

  /* get the command and argument. */
  if (!sscanf(data, "%s %s", cmd, arg))
    {
      cmd = data;
    }

  /* lower_case the command... */
  cmd = lower_case(cmd);

  /* If we're not connected, these are valid commands... */
  if (!thisSession->connected)
    {
      switch(cmd)
	{
	case "user":
	  FTP_CMD_user(thisSession, arg);
	  return;
	case "pass":
	  FTP_CMD_pass(thisSession, arg);
	  return;
	case "quit":
	  FTP_CMD_quit(thisSession, arg);
	  return;
	case "noop":
	  FTP_CMD_noop(thisSession, arg);
	  return;
	default:
	  socket->send("503 Log in with USER first.\n");
	  return;
	}
    }

  /* We are connected, so dispatch to the proper handler. */
  dispatchTo = dispatch[cmd];
  if (!dispatchTo)
    {
      /* Log command so we know what clients are trying to use */
      socket->send(sprintf("502 Unknown command %s.\n", cmd));
      return;
    }
  if(catch(evaluate(dispatchTo, thisSession, arg)))
     {
       FTPLOGF("%s caused a FAILURE with command '%s'.\n",
	    capitalize(thisSession->user), data);
       socket->send("550 Unknown failure.  Please report what you were doing "
		    "to the mud admin.\n");
     }
  return;
}

private void FTP_close(object socket)
{
  map_delete(sessions, socket);
}

private void create()
{
  sock = new(SOCKET, SKT_STYLE_LISTEN, PORT_FTP, (: FTP_read :),
	     (: FTP_close :));
}

private void FTP_DATA_read(object socket, mixed text)
{
  class ftp_session	info;

  info = sessions[dataports[socket]];

  switch(info->binary)
    {
    case 0:
      text=replace_string(text,"\r","");
      unguarded(info->priv, (:write_file($(info->targetFile), $(text)):));
      return;
    case 1:
      unguarded(info->priv, (:write_buffer($(info->targetFile), $(info->filepos), $(text)):));
      info->filepos+=sizeof(text);
      return;
    default:
      ENSURE(0);
    }
}

private void FTP_DATA_close(object socket)
{
  class ftp_session	info;

  info = sessions[dataports[socket]];

  if(info)
    {
      info->cmdPipe->send("226 Transfer complete.\n");
    }
}

private void FTP_CMD_user(class ftp_session info, string arg)
{
  NEEDS_ARG();
  arg = lower_case(arg);
  if(info->connected)
    {
      info->cmdPipe->send(sprintf("530 User %s access denied.\n", arg));
      return;
    }
  info->user = arg;

#ifdef ALLOW_ANON_FTP
  if(member_array(arg, anon_logins) != -1)
    {
      info->cmdPipe->send("331 Guest login ok, send your complete e-mail "
			  "address as password.\n");
      return;
    }
#endif

  info->cmdPipe->send(sprintf("331 Password required for %s.\n", arg));
  return;
}

private void FTP_CMD_pass(class ftp_session info, string arg)
{
  string	password;
  mixed	array userinfo;

  NEEDS_ARG();
  if(info->connected || !info->user)
    {
      info->cmdPipe->send("503 Login with USER first.\n");
      return;
    }

#ifdef ALLOW_ANON_FTP
  if(ANON_USER())
    {
      info->cmdPipe->send("230 guest login ok, access restrictions apply.\n");
      info->connected = 1;
      info->priv = 0;
      info->pwd = ANON_PREFIX;
      FTPLOGF("Anomymous login from %s (email = %s)\n", 
	      info->cmdPipe->address()[0], arg);
      return;
    }
#endif

  userinfo = unguarded(1,(:USER_D->query_variable($(info->user), 
						  ({"password"})):));
  if(arrayp(userinfo) && sizeof(userinfo))
    {
      password = userinfo[0];
    }
  else
    {
      info->cmdPipe->send("530 Login incorrect.\n");
      return;
    }
  if(crypt(arg, password) != password || !wizardp(info->user))
    {
      info->cmdPipe->send("530 Login incorrect.\n");
      return;
    }
  info->cmdPipe->send(sprintf("230 User %s logged in.\n", info->user));
  info->connected = 1;
  FTPLOGF("%s connected from %s.\n", capitalize(info->user), 
	      info->cmdPipe->address()[0]);

  if(adminp(info->user))
    {
      info->priv = 1;
    }
  else
    {
      info->priv = info->user;
    }
  info->pwd = join_path(WIZ_DIR,info->user);
  return;
}

private void FTP_CMD_quit(class ftp_session info, string arg)
{
  info->cmdPipe->send("221 Goodbye.\n");
  if(info->dataPipe)
    {
      destruct(info->dataPipe);
    }
  FTPLOGF("%s QUIT at %s.\n", capitalize(info->user), ctime(time()));
  map_delete(sessions, info->cmdPipe);
  destruct(info->cmdPipe);

}

private void FTP_CMD_port(class ftp_session info, string arg)
{
  string 	ip, *parts;
  int		port;
  NEEDS_ARG();

  parts = explode(arg, ",");
  if(sizeof(parts)!=6)
    {
      info->cmdPipe->send("550 Failed command.\n");
      destruct(info->dataPipe);
      return;
    }
  ip = implode(parts[0..3],".");
  // Make a 16 bit port # out of 2 8 bit values. 
  port = (to_int(parts[4]) << 8) + to_int(parts[5]);
  if(info->dataPipe)
    {
      destruct(info->dataPipe);
    }
  switch(info->binary)
    {
    case 0:
      info->dataPipe = unguarded(1,(:new(SOCKET, SKT_STYLE_CONNECT, 
					 sprintf("%s %d", $(ip), $(port)),
				    (: FTP_DATA_read :),
				    (: FTP_DATA_close :) ):));
      break;
    case 1:
      info->dataPipe = unguarded(1,(:new(SOCKET, SKT_STYLE_CONNECT_B, 
					 sprintf("%s %d", $(ip), $(port)),
				    (: FTP_DATA_read :),
				    (: FTP_DATA_close :) ):));
      break;
    default:
      return;
    }
  // map the data port to the cmd port so we can find the cmd port when
  // we're given the data port.
  dataports[info->dataPipe] = info->cmdPipe;
  info->dataPipe->set_write_callback((:FTP_write:));
  info->cmdPipe->send("200 PORT command successful.\n");
  return;
}

private void FTP_CMD_nlst(class ftp_session info, string arg)
{
  string array 	files;

  if(!arg || arg == "")
    {
      arg = ".";
    }
  arg = evaluate_path(arg, info->pwd);

  ANON_CHECK(arg);

  if(unguarded(1, (:is_directory($(arg)):)))
    {
      arg = join_path(arg, "*");
    }
  if(unguarded(info->priv, (:file_size(base_path($(arg))):)) == -1)
    {
      info->cmdPipe->send(sprintf("550 %s: No such file OR directory.\n", arg));
      destruct(info->dataPipe);
      return;
    }

  files = unguarded(info->priv, (:get_dir($(arg)):));
  if(!files)
    {
	  info->cmdPipe->send(sprintf("550 %s: Permission denied.\n", arg));
	  destruct(info->dataPipe);
	  return;
    }
  files -= ({".",".."});
  if(!sizeof(files))
    {
      info->cmdPipe->send("550 No files found.\n");
      destruct(info->dataPipe);
      return;
    }
  info->cmdPipe->send("150 Opening ascii mode data connection for "
		      "file list\n");

  info->dataPipe->send(implode(files, "\r\n") + "\r\n");
}

private void FTP_CMD_list(class ftp_session info, string arg)
{
  string array 	files;
  string array  output;

  if(!arg || arg == "")
    {
      arg = ".";
    }
  arg = evaluate_path(arg, info->pwd);
  
  ANON_CHECK(arg);

  if(unguarded(1, (:is_directory($(arg)):)))
    {
      arg = join_path(arg, "*");
    }
  if(unguarded(info->priv, (:file_size(base_path($(arg))):)) == -1)
    {
      info->cmdPipe->send(sprintf("550 %s: No such file OR directory.\n", arg));
      destruct(info->dataPipe);
      return;
    }
  files = unguarded(info->priv, (:get_dir($(arg),-1):));
  if(!files)
    {
	  info->cmdPipe->send(sprintf("550 %s: Permission denied.\n",arg));
	  destruct(info->dataPipe);
	  return;
    }
  files = filter(files, (: member_array($1[0], ({".",".."})) == -1 :));
  if(!sizeof(files))
    {
      info->cmdPipe->send("550 No files found.\n");
      destruct(info->dataPipe);
      return;
    }

  output = map(files, (:sprintf("%s %=9s %s %s", $1[1] == -2 ? "drwxrwsr-x" : "-rwxrw-r--",
         $1[1] == -2 ? "/" : sprintf("%d", $1[1]), ctime($1[2])[4..], $1[0])
		       :));
  info->cmdPipe->send("150 Opening ascii mode data connection for file list\n");
  info->dataPipe->send(implode(output, "\r\n")+"\r\n");
  return;
}

private void FTP_CMD_retr(class ftp_session info, string arg)
{
  string	target_file;
  string	strToSend;
  buffer	bufToSend;
  int           i;

  NEEDS_ARG();
  
  target_file = evaluate_path(arg, info->pwd);

  ANON_CHECK(target_file);

  if(unguarded(info->priv, (:is_directory($(target_file)):)))
    {
      info->cmdPipe->send(sprintf("550 %s: Can't retrieve (it's a directory)."
				  "\n", target_file));
      destruct(info->dataPipe);
      return;
    }
  if(!unguarded(info->priv, (:is_file($(target_file)):)))
    {
      info->cmdPipe->send(sprintf("550 %s: No such file OR directory.\n", 
				  target_file));
      destruct(info->dataPipe);
      return;
    }
  if(!unguarded(info->priv, (:file_size($(target_file)):)))
    {
      info->cmdPipe->send(sprintf("550 %s: File contains nothing.\n",
				  target_file));
      destruct(info->dataPipe);
      return;
    }
  
  switch(info->binary)
    {
    case 0:	
      i=file_size(target_file);
      FTPLOGF("%s GOT %s.\n", capitalize(info->user), target_file);

      outfile[info->dataPipe]=({target_file,0,0,info->cmdPipe});
      info->dataPipe->set_write_callback((: FTP_CMD_retr_callback :));

      info->cmdPipe->send(sprintf("150 Opening ascii mode data connection for "
				  "%s (%d bytes).\n", target_file, i));

      info->dataPipe->send(FTP_CMD_retr_callback(info->dataPipe));
      break;
    case 1:
      i=file_size(target_file);

      outfile[info->dataPipe]=({target_file,1,0,info->cmdPipe});
      info->dataPipe->set_write_callback((: FTP_CMD_retr_callback :));

      info->cmdPipe->send(sprintf("150 Opening binary mode data connection "
				  "for %s (%d bytes).\n", target_file, i));

      info->dataPipe->send(FTP_CMD_retr_callback(info->dataPipe));
      break;
    default:
      ENSURE(0);
    }
}

void FTP_CMD_pwd(class ftp_session info, string arg)
{
  info->cmdPipe->send(sprintf("257 \"%s\" is current directory.\n", info->pwd));
}

void FTP_CMD_noop(class ftp_session info, string arg)
{
    info->cmdPipe->send("221 NOOP command successful.\n");
}

private void FTP_CMD_stor(class ftp_session info, string arg)
{

  NEEDS_ARG();

  arg = evaluate_path(arg, info->pwd);


#ifndef ANON_CAN_PUT
#ifdef ALLOW_ANON_FTP
  if(ANON_USER())
    {
      info->cmdPipe->send("550 Permission denied.\n");
      destruct(info->dataPipe);
      return;
    }
#endif
#else
  ANON_CHECK(arg);
#endif

  if(!unguarded(info->priv,(:is_directory(base_path($(arg))):)))
    {
      info->cmdPipe->send("553 No such directory to store into.\n");
      destruct(info->dataPipe);
      return;
    }
  info->targetFile = arg;
  if(unguarded(info->priv, (:is_file($(arg)):)))
    {
      if(!unguarded(info->priv, (:rm($(arg)):)))
	{
	  info->cmdPipe->send(sprintf("550 %s: Permission denied.\n", arg));
	  destruct(info->dataPipe);
	  return;
	}
    }
  else if(!unguarded(info->priv, (:write_file($(arg), ""):)))
    {
      info->cmdPipe->send(sprintf("550 %s: Permission denied.\n", arg));
      destruct(info->dataPipe);
      return;
    }
  FTPLOGF("%s PUT %s.\n", capitalize(info->user), arg);

  // Reset the file position flag.
  info->filepos=0;
  info->cmdPipe->send(sprintf("150 Opening %s mode data connection for %s.\n",
			      info->binary ? "binary" : "ascii",
			      arg));
}

private void FTP_CMD_cdup(class ftp_session info, string arg)
{
  FTP_CMD_cwd(info, "..");
}

private void FTP_CMD_cwd(class ftp_session info, string arg)
{
  string	newpath;

  NEEDS_ARG();
  newpath = evaluate_path(arg, info->pwd);

  ANON_CHECK(newpath);

  if(!unguarded(info->priv, (:is_directory($(newpath)):)))
    {
      info->cmdPipe->send(sprintf("550 %s: No such file or directory.\n", 
				  newpath));
      return;
    }
  info->pwd = newpath;
  info->cmdPipe->send("250 CWD command successful.\n");
}

private void FTP_CMD_mkd(class ftp_session info, string arg)
{
  NEEDS_ARG();

  arg = evaluate_path(arg, info->pwd);

#ifndef ANON_CAN_PUT
#ifdef  ALLOW_ANON_FTP
  if(ANON_USER())
    {
      info->cmdPipe->send("550 Permission denied.\n");
      destruct(info->dataPipe);
      return;
    }
#endif
#else
  ANON_CHECK(arg);
#endif

  if(!unguarded(info->priv, (:is_directory(base_path($(arg))):)))
    {
      info->cmdPipe->send(sprintf("550 %s: No such directory.\n",
				  base_path(arg)));
      return;
    }
  if(unguarded(info->priv, (:file_size($(arg)):)) != -1)
    {
      info->cmdPipe->send(sprintf("550 %s: File exists.\n", arg));
      return;
    }

  if(!unguarded(info->priv, (:mkdir($(arg)):)))
    {
      info->cmdPipe->send(sprintf("550 %s: Permission denied.\n", arg));
      return;
    }
  info->cmdPipe->send("257 MKD command successful.\n");
  return;
}

private void FTP_CMD_type(class ftp_session info, string arg)
{
  NEEDS_ARG();
  switch(arg)
    {
    case "a":
    case "A":
      info->binary = 0;
      info->cmdPipe->send("200 Type set to A.\n");
      return;
    case "i":
    case "I":
      info->binary = 1;
      info->cmdPipe->send("200 Type set to I.\n");
      return;
    default:
      info->cmdPipe->send("550 Unknown file type.\n");
      return;
    }
}

private void FTP_CMD_dele(class ftp_session info, string arg)
{
  NEEDS_ARG();
  arg = evaluate_path(arg, info->pwd);

  ANON_CHECK(arg);

  if(!unguarded(info->priv, (: is_file($(arg)):)))
    {
      info->cmdPipe->send(sprintf("550 %s: No such file OR directory.\n", arg));
      return;
    }
  if(!unguarded(info->priv, (: rm($(arg)) :)))
    {
      info->cmdPipe->send(sprintf("550 %s: Permission denied.\n",arg));
      return;
    }
  FTPLOGF("%s DELETED %s.\n", capitalize(info->user), arg);
  info->cmdPipe->send("250 DELE command successful.\n");
}

void remove()
{
  class ftp_session item;

  foreach(item in values(sessions))
    {
      if(objectp(item->cmdPipe))
	{
	  destruct(item->cmdPipe);
	}
      if(objectp(item->dataPipe))
	{
	  destruct(item->dataPipe);
	}
    }

  destruct(sock);

  remove_call_out("FTP_handle_idlers");
}

int clean_up() {
 return 0;
}

string array list_users()
{
   return map(values(sessions), (: ((class ftp_session)$1)->connected ? 
				((class ftp_session)$1)->user : "(login)" :));
}

private void FTP_CMD_syst(class ftp_session info, string arg) {
 info->cmdPipe->send("215 UNIX Mud Name: "+mud_name()+"\n");
}

string FTP_CMD_retr_callback(object ob) {
 function f;
 string s;
 int start,length;
 mixed ret;

 if (!ob || undefinedp(outfile[ob])) return 0;

 start=outfile[ob][2];
 length=FTP_BLOCK_SIZE;
 outfile[ob][2]+=length;

 if (start+length>file_size(outfile[ob][0])) length=file_size(outfile[ob][0])-start;

 ret=read_buffer(outfile[ob][0],start,length);

 if (start+length>=file_size(outfile[ob][0])) {
  map_delete(outfile,ob);
  ob->set_write_callback((:FTP_write:));
 }

 return ret;
}