/* Do not remove the headers from this file! see /USAGE for more info. */

/*
** naming.c -- functions for naming and description of the player body
**
** 960603, Deathblade: created.
*/

#include <commands.h>		/* for CMD_OB_xxx */

private string name = "guest";
private string describe;
private string invis_name;
private string nickname;
private static string cap_name;

int is_visible();
int test_flag(int which);
int query_ghost();
object query_link();
string number_of(int num, string what);
void save_me();
string query_reflexive();
void remove_id(string array id...);
void add_id_no_plural(string array id...);
string in_room_desc();

#ifdef USE_TITLES
string query_title();
#endif

int query_prone();

string query_name()
{
    if ( invis_name == cap_name || !invis_name ) invis_name = "Someone";
    if ( !is_visible() ) return invis_name;
    return cap_name;
}

string query_long_name()
{
    if (query_ghost())
	return "The ghost of " + cap_name;
#ifdef USE_TITLES
    return query_title();
#else
    return cap_name;
#endif
}

nomask string query_userid()
{
    return name;
}

nomask string query_invis_name()
{
    return invis_name;
}

string query_idle_string()
{
    object link = query_link();
    int idle_time;
    string result = "";

    if ( interactive(link) )
	idle_time = query_idle(link);
    if ( idle_time < 60 )
      return "";

    result += " [idle " + convert_time(idle_time, 2) + "]";

    return result;
}

// This is used by in_room_desc and by who, one of which truncates,
// one of which doesnt.  Both want an idle time.
string base_in_room_desc()
{
    string result;
    object link = query_link();

    result = query_long_name();

    if (query_prone())
        return cap_name + " is lying here slumped on the ground.";

    /* if they are link-dead, then prepend something... */
    if ( !link || !interactive(link) )
	result = "The lifeless body of " + result;

    return result;
}

string query_formatted_desc(int num_chars)
{
    string idle_string;
    int i;
  
    idle_string = query_idle_string();
    if ( i = strlen(idle_string) )
    {
	num_chars -= (i + 1);
	idle_string = " " + idle_string;
    }
    return M_ANSI->colour_truncate(base_in_room_desc(), num_chars) + idle_string;
}

void set_description(string str)
{
    if(base_name(previous_object()) == CMD_OB_DESCRIBE)
	describe = str;
    save_me();
}

string our_description()
{
    if (describe)
	return in_room_desc() + "\n" + describe +"\n";

    return in_room_desc() + "\n" + cap_name + " is boring and hasn't described " + query_reflexive() + ".\n";
}

void set_nickname(string arg)
{
    if (file_name(previous_object()) != CMD_OB_NICKNAME)
	error("Illegal call to set_nickname\n");

    if ( nickname )
	remove_id(nickname);

    nickname = arg;
    add_id_no_plural(nickname);
}

string query_nickname()
{
    return nickname;
}

static void naming_create(string userid)
{
    cap_name = capitalize(userid);
    name = userid;
}

static void naming_init_ids()
{
    add_id_no_plural(name);

    if ( nickname )
	add_id_no_plural(nickname);
}
