/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* variable.h

	Header file for defining variable structures.

	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

struct Variable {
    char   *name;
    int     type;		/* what type of variable is this */
    int     max_x,
            max_y;		/* what size is it */
    int    *varptr;
    struct Variable *next;
};

void install_var ();
struct Variable *lookup_var ();
boolean change_variable_length ();

/* here we define the type of types available for variables */

#define	Char	0	
#define	Int	1
#define Float	2
#define Double	3
#define String	4		/* vector of chars */
#define Vint	5		/* vector of integers */
#define Vfloat	6		/* vector of floats */
#define Vdouble	7		/* vector of doubles */
#define Vstring 8		/* vector of strings */
#define PVfloat 9		/* pointers to vectors of floats
				   needed for ipatterns and tpatterns */

#define PVweight 10		/* pointers to vectors of floats
				   with access to first_weights, num_weights */

extern char **uname;
extern int nunames;
extern int nunits;
extern int *first_weight_to;
extern int *num_weights_to;
extern boolean System_Defined;
extern int define_system();
