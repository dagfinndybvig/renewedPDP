/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* template.h

	Header file for reading templates.

	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

struct Template {
    char   *name;
    int     type;		/* type: from list that follows this
				   definition */
    int     display_level;	/* necessary level to display item */
    int     x,
            y;			/* starting point for x and y */
            boolean orientation;/* on for horizontal, off for vertical */
    int     min_x,
            min_y;		/* for use by vectors and arrays */
    int     max_x,
            max_y;		/* for use by vectors and arrays */
    int     digits;		/* number of digits to display */
    float   precision;		/* scale factor of precision */
    int	    spacing;		/* spacing between successive items on
				   orientation dimension */
    struct Look *look;		/* pointer to the look */
    struct Variable *var;	/* pointer to the actual variable */
    int     defined;		/* 0 = no, 1 = yes, -1 = label */
    struct Template *next;	/* pointer to the next one */
};

extern struct Template *tlist;
extern struct Template **torder;
extern int ntemplates;

/* these are the various types of displays that are supported */

#define COMMAND		30	/* the command line */
#define	VECTOR		31	/* a vector of numbers */
#define	MATRIX		32	/* a matrix of numbers */
#define	LABEL		34	/* a character string */
#define	LABEL_ARRAY	35	/* an array of character strings */
#define	VARIABLE	36	/* one of the basic variables */
#define LOOK		38	/* get the format from a look file */
#define	FLOATVAR	39	/* float variable printed as Float */
#define LABEL_LOOK      41      /* labels formated from a look file */

#define HORIZONTAL	TRUE
#define VERTICAL	FALSE

#define MAXSLOTS	100

struct Look {
    int     look_x,
            look_y;		/* rows and columns in the look matrix */
    char    **look_template;	/* pointer to the array */
};

#define NOCELL		NULL	/* marker meaning there is no value here */

extern boolean	layout_defined;
extern char	**background;
