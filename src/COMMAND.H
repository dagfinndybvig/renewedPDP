/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* command.h

	Structures and tables for the command interface.
	
	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

#include <signal.h>

#define NOMENU		-999
#define BASEMENU	1
#define IOMENU		2
#define GETMENU		3
#define SAVEMENU	4
#define DISPLAYMENU	5
#define DISPLAYOPTIONS	6
#define SETMENU		7
#define SETCONFMENU	8
#define SETMODEMENU	9
#define SETENVMENU	10
#define SETPARAMMENU	11
#define SETSVMENU	13
#define SETPCMENU	SETMENU
#define SETWTMENU	SETMENU
#define MAXCOMMANDS	100 /* this is the start value for maxcommands */

extern int maxcommands; 
/* maxcommands is declared in command.c.  It starts out
equal to MAXCOMMANDS and is automatically increased if
more commands are installed with install_command */

extern char *Prompt;

struct Command_table {
    char   *command;		/* the name of the command */
    int     (*func)();
    int     menutype;		/* integer specifying which menu this is on */
    int    *arg;		/* to pass around arguments !saves funcalls */
};

extern struct Command_table *Command;
void     install_command ();

/* The table holds the main command interface information.
   It consists of a name for the command, followed by a
   pointer to a function that implements the command.
   Each function is passed the command string that caused
   it to be called.  The table is malloc'd in the function
   init_general(). 
*/

int     do_command ();
int     do_help ();
