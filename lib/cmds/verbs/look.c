/* Do not remove the headers from this file! see /USAGE for more info. */

/*
** look.c
**
*/

#include <mudlib.h>

inherit VERB_OB;

mixed can_look(string rule)
{
    return 1;
}

mixed can_look_obj(object ob) {
    return 1;
}

mixed can_look_at_obj(object ob) {
    return 1;
}

mixed can_look_for_liv(object liv) {
    return 1;
}

mixed can_look_at_obj_with_obj(object ob1, object ob2) {
    return 1;
}

mixed can_look_str(string str) {
    mapping exits;

    exits = environment(this_body())->get_exits();
    if (stringp(exits[str]))
	return 1;
    return "That doesn't seem to be possible.\n";
}

mixed can_look_obj_with_obj(object ob1, object ob2) {
    return 1;
}

void do_look() {
    environment(this_body())->do_looking();
}

void do_look_at_obj(object ob, string name) {
    string str;
    if (!(str = ob->get_item_desc(name)))
	str = ob->long();
    if (str[<1] != '\n') str += "\n";
    write(str);
}

void do_look_in_obj(object ob, string name) {
    ob->look_in(name);
}

void do_look_obj(object ob, string name) {
    do_look_at_obj(ob, name);
}

void do_look_at_obj_with_obj(object o1, object o2) {
    // o2->indirect_look_at_obj_with_obj() has already indicated this is ok.
    o2->use("look", o1);
}

void do_look_obj_with_obj(object o1, object o2) {
    do_look_at_obj_with_obj(o1, o2);
}

void do_look_for_liv(object ob) {
    if (environment(this_body()) != environment(ob)) {
	// I don't think this can happen
	write("Search around a bit ...\n");
    } else {
	this_body()->my_action("$O is right here!\n", ob);
    }
}

void do_look_for_obj(object ob) {
    /* ### actually, OBJ has to be here, right? */
    /* Beek: No, it can be in a bag/bottle etc.  Still, this should be smarter*/
    if (environment(this_body()) == environment(ob)) {
	this_body()->my_action("$O is right here!\n", ob);
    } else 
      write("I'm sure that's around here somewhere...\n");
}

void do_look_str(string str) {
    mapping exits = environment(this_body())->get_exits();
    string fn;
    object ob;

    fn = exits[str];
    if (!stringp(fn)) return;
    ob = load_object(fn);

    ob->remote_look(environment(this_body()));
}

#if 0
mixed do_look(mixed rule, mixed ob1, object ob2)
{
    string *	exits;

    switch (rule) {
    case "at OBS":
    case "at OBS with OBJ":
	return "Try looking at just one thing at a time.\n";
    }

}
#endif

mixed * query_verb_info()
{
    return ({ ({ 0, "OBJ", "at OBJ", "for LIV", "for OBJ", "in OBJ",
	     "at OBS", "at OBS with OBJ", "at OBJ with OBJ",
	     "STR", "OBJ with OBJ" }) });
    
    /*
    ** "examine OBJ" -> "look OBJ"
    ** "examine OBS" -> "look OBS"
    ** "examine OBS with OBJ" -> "look OBS with OBJ"
    ** "examine OBJ with OBJ" -> "look OBJ with OBJ"
    ** "gaze at OBJ" -> "look at OBJ"
    ** "gaze at OBS" -> "look at OBS"
    ** "gaze at OBS with OBJ" -> "look at OBS with OBJ"
    ** "gaze at OBJ with OBJ" -> "look at OBJ with OBJ"
    ** "gaze STR" -> "look STR"
    ** "gaze up" -> "look up"
    ** "gaze down" -> "look down"
    ** "stare 1" -> "look 1"
    ** "stare at OBJ" -> "look at OBJ"
    ** "stare at OBS" -> "look at OBS"
    ** "stare at OBS with OBJ" -> "look at OBS with OBJ"
    ** "stare at OBJ with OBJ" -> "look at OBJ with OBJ"
    ** "stare STR" -> "look STR"
    ** "stare up" -> "look up"
    ** "stare down" -> "look down"
    */
}


#ifdef OLD_CODE

#include <mudlib.h>

inherit M_PARSING;

static private string * mypreps;

create()
{
    mypreps = ({"in","behind","under","with", "on", "down", "around" });
}

int look( int rule, mixed stack, mixed input )
{
    mapping ex;
    object* obs;

    switch( rule )
    {
    case 1:
	environment( this_body() )->do_looking();
	return 1;
    case 2:
	if( !stringp( stack[2] ) )
	    write("Right here!\n");
	else
	    if( find_player( stack[2] ) )
		printf("Search around a bit...\n");
	    else
		write("I'm sure that's around here somewhere...\n");
	return 1;
    case 3:
	if( sizeof( stack ) == 5 && stack[1] == "at" &&
	   (objectp(stack[2]) || pointerp(stack[2])) && stringp( stack[3] )
	   && objectp( stack[4] ) )
	{
	    mixed p;

	    p = condense_phrase( stack[2..4] );
	    if( !pointerp( p ) )
		return p;

	    if( sizeof( p ) != 1 )
		return write( "Try just looking at one thing "
				   "at a time.\n" ), 0;

	    return write( stack[2]-> long() ), 1;

	}
	if( sizeof( stack ) != 3 )
	    return 0;
	if( !objectp( stack[2] ) )
	    return 0;
	if( member_array(stack[1], mypreps) == -1)
	    return 0;

	return write( stack[2]->look_in(stack[1]) ), 1;
    case 4:
	stack = ({""}) + stack;

    case 5:
	if( stack[2] == environment( this_body() ) )
	    return write(
			 environment(this_body())->show_item_desc(input[8..])),1;
	return write( stack[2]->long() ), 1;
    case 6:
	write( "Try just looking at one thing at a time.\n" );
	return 0;
    case 11:
	stack = ({""})+stack;
    case 7:
	return write( stack[4]->use("look", stack[2] ) ), 1;
    case 10:
	ex = environment(this_body())->get_exits();
	if ( member_array(ex, stack[1]) )
	{
	    write(find_object(ex[stack[1]])->
		      remote_look(environment(this_body())));
	    return 1;
	}

	write( "That doesn't seem to be possible.\n" );
	return 1;
    }
}

#endif /* OLD_CODE */