/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* file: pa.h

	Header file for the pa program.
	
	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

extern char *prompt;
extern  boolean System_defined;
extern int  	epoch;
extern float   *netinput;
extern float   *output;
extern float   *target;
