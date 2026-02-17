/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* display.h

	Header file for display functions.

	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

extern int  command_x;
extern int  command_y;
extern int  Display_level;
extern int  stand_out;

char   *get_command ();
int     update_display ();
int     reset_display ();
int     change_display ();
int     do_update_template ();

#define MAX_SCREEN_LINES	 23
#define MAX_SCREEN_COLUMNS	 79

extern int num_lines;
extern int num_cols;
