/* Do not remove the headers from this file! see /USAGE for more info. */

inherit "/std/indoor_room";

void setup();

void setup() {
function f;
set_exits( ([
  "west" : "forest10.scr",
  "north" : "forest4.scr",
]) );
set_brief("Deep forest");
set_long("You are in the deep forest.");
set_light(1);
}

