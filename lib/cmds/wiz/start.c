/* Do not remove the headers from this file! see /USAGE for more info. */

// Rust

#include <mudlib.h>




int main(mixed *arg) {
    string where;
    if (!arg[0]) {
	printf("You start at: %s\n", this_body()->query_start_location() );
	return 1;
    }
    if( arg[0]->is_living() )
    {
	write("Yeah, right.\n");
	return 0;
    }
    where = file_name(arg[0]);
    this_body()->set_start_location( where );
    printf("Ok, you now start at: %s.\n", where );
    return 1;
}
