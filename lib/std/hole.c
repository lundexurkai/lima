/* Do not remove the headers from this file! see /USAGE for more info. */


inherit CONTAINER;

void
setup() 
{

    set_id("hole");
    set_long("There isn't anything special about the hole.\n");
    add_relation("in",LARGE);
    set_default_relation("in");
    set_size(MEDIUM);
}


void change_desc(string s)
{
    set_in_room_desc(s);
}

void change_max_capacity(int x)
{
    set_max_capacity(x);
}
