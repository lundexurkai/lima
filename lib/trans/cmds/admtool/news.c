/* Do not remove the headers from this file! see /USAGE for more info. */

/*
** news.c -- News administration
**
** 950715, Rust: created
*/

#include <mudlib.h>

void std_handler(string str);
varargs void modal_simple(function input_func, int secure);

static nomask void write_news_menu()
{
    write("Administration Tool: news administration\n"
	  "\n"
	  "    l        - list newsgroups\n"
	  "    a        - add a newsgroup\n"
	  "    r <name> - remove a newsgroup\n"
	  "\n"
	  "    m        - main menu\n"
	  "    q        - quit\n"
	  "    ?        - help\n"
	  "\n"
	  );
}

private nomask void list_groups()
{
    string* grouplist = NEWS_D->get_groups();

    if ( sizeof(grouplist) == 0 )
    {
	write("    <none>\n");
	return;
    }
    grouplist = sort_array(grouplist, 1);
    new(MORE_OB)->more_string(implode(grouplist,"\n"));

}


private nomask void rcv_group_name(string str)
{
    str = lower_case(trim_spaces(str));

    if ( str == "" )
	return;

    if ( member_array(str,NEWS_D->get_groups()) != -1)
    {
	write("** That group already exists.\n");
	return;
    }

    NEWS_D->add_group(str);
    write("** group added.\n");
}

private nomask void add_group()
{
    write("New group name? ");
    modal_simple((: rcv_group_name :));
}

private nomask void remove_group(string group_name)
{
    string* grouplist = NEWS_D->get_groups();
    
    if(!group_name)
      {
	write("** no group name supplied.\n");
	return;
      }
    group_name = lower_case(group_name);
    if ( member_array(group_name, grouplist) == -1 )
    {
	write("** That newsgroup does not exist.\n");
	return;
    }

    NEWS_D->remove_group(group_name);
    write("** group removed.\n");
}

static nomask void receive_news_input(string str)
{
    string name;

    sscanf(str, "%s %s", str, name);

    switch ( str )
    {
    case "l":
	list_groups();
	break;

    case "a":
	add_group();
	break;

    case "r":
	remove_group(name);
	break;

    case "?":
	write_news_menu();
	break;

    default:
	std_handler(str);
	break;
    }
}