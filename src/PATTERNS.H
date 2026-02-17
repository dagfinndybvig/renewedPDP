/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* file: patterns.h

	This file contains the header information for patterns.
	
	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

#define MAXPATTERNS	100
#define	PAIRS	1

extern int  npatterns;
extern int  maxpatterns;
extern float **ipattern;
extern float **tpattern;
extern char **pname;
extern int *used;
extern char cpname[];

int     get_pattern_pairs ();
int     get_patterns ();
int	init_pattern_pairs ();
int	init_patterns ();
