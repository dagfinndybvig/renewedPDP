/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* File: weights.h

	header file for reading in weights
	
	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

#define RESET_WEIGHTS 1
#define INIT_WEIGHTS  0
#define MAXCONSTRAINTS 100

extern int  ninputs;
extern int  nunits;
extern int  noutputs;
extern int  nlinks;
extern int  epsilon_menu;

extern float    lrate;
extern float   **weight;
extern float   **wed;
extern char    **wchar;
extern int	*first_weight_to;
extern int	*num_weights_to;
extern float   *bias;
extern float   *bed;
extern char    *bchar;
extern float   **epsilon;
extern float   *bepsilon;
extern float   *sigma;

extern float	wrange;
extern int 	epochno;

extern float  **positive_constraints;
extern float  **negative_constraints;

struct constants {
    int     random;
    int     positive;
    int     negative;
    int     link;
    float   value;
};

struct constraint {
    int     num;
    int	    max;
    float   **cvec; /* these are the weights */
    float   **ivec; /* these are the increments */
};

extern struct constraint   *constraints;
extern struct constants	constants[26];

int     define_network ();
