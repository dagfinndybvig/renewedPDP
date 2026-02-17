/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/

/*
	Just defines main() -- try command.c for the command interface.
    
	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

#include "general.h"
#include "variable.h"
#include "command.h"
#include "patterns.h"

boolean	start_up = 1;

char    err_string[LINE_SIZE];
char	prog_name[10];
char	version[10] = "1.1";

main(argc, argv)
int     argc;
char  **argv;
{

    char   *get_command ();
    
    get_progname();

    printf("      Welcome to %s, a PDP program (Version %s).\n",
    		prog_name,version);
    printf("Copyright 1987 by J. L. McClelland and D. E. Rumelhart.\n");
    sleep(3);
    (void) srand(time(0));
    random_seed = rand();
    (void) srand(random_seed); /* we now have a restartable random seed */
    init_general();
    init_display();
    init_system();

    start_up = 1;

    if (argc-- > 1) {
        ++argv;
	if (argv[0][0] == '-'){
		/* user wants to skip the template file */
	}
    else if ((in_stream = fopen_read_compat(*argv)) == NULL) {
	    sprintf(err_string,"cannot open %s\n", *argv);
	    put_error(err_string);
	}
	else {
	    read_template();
	    fclose(in_stream);
	}
	if (argc-- > 1) {
        if ((in_stream = fopen_read_compat(*++argv)) == NULL) {
		sprintf(err_string,"cannot open %s\n", *argv);
		put_error(err_string);
	    }
	    else {
		while(! feof(in_stream))
		   do_command(Prompt, (int *) BASEMENU);
	    }
	    fclose(in_stream);
	}
    }

    if (!System_Defined && nunits > 0) {
        (void) define_system();
    }
    
    redisplay();    /* tests templates for validity */

    in_stream = stdin;

    start_up = 0;
    
    io_initscr();		/* initialize the screen */
    redisplay();		/* put up initial display */

    while (TRUE) {
	do_command(Prompt, (int *) BASEMENU);
    }
}

get_progname() {
    int i;
    
    i = 0;
    
    while (Prompt[i] && Prompt[i] != ':') {
    	prog_name[i] = Prompt[i];
	i++;
    }
    prog_name[i] = '\0';
}
