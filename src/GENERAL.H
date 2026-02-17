/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* general.h

	Some useful and general definitions for the PDP-PC package.

	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

#include <stdio.h>
#include <ctype.h>

#ifdef MSDOS
#define Interrupt (kbhit() == -999) || Interrupt_flag
/* "Interrupt" is used to check whether the interrupt flag has been
set by int_handler.  The call to kbhit is necessary for MSDOS to check whether
the user has typed a control-C.  If a control-C has been typed, MSDOS will
generate an interrupt, which will set the Interrupt flag (before the flag
is evaluated).  kbhit() will never return -999; the call is there only to
cause an interrupt-check.  Removing the kbhit call would render control-C
ineffective. */
#else
#define Interrupt Interrupt_flag
#endif


#ifdef MSDOS
#include <string.h>

#include <conio.h>  /* contains declaration of kbhit() */

extern int	sprintf();
extern char	*index(); /* defined in general.c */
extern char 	xxtemp[200];
extern int 	io_type; /* normal or reverse video */

#else
#include <strings.h>
#endif
#include "display.h"

#define	FALSE	(0)
#define TRUE	(1)
#define BREAK		(1)
#define CONTINUE	(0)
#define POP		(2)

#define UPDATE	0
#define CYCLE   1
#define NCYCLES 2
#define PATTERN 3
#define EPOCH   4
#define NEPOCHS	5

#define LINE_SIZE BUFSIZ

extern  char err_string[LINE_SIZE];
typedef int boolean;

int     quit ();
char   *emalloc ();
char   *erealloc ();
char   *strsave ();
FILE   *fopen_read_compat ();
double  atof ();

int     rand ();

#ifdef MSDOS

#define strindex strchr
/* #define rnd() ((float)rand()/32767.0) faster to multiply -- jlm */
#define rnd()	((float)rand()*3.0518507e-5) 

#else

/* #define rnd() ((float)rand()/2147483647.0) faster to multiply -- jlm */
#define rnd() ((float)rand()*0.4656612875e-9) 

#define strindex index

#endif

#define STRINGLENGTH	40

extern  FILE * in_stream;	/* the general input stream */
extern int  Interrupt_flag;
extern int  single_flag;
extern int  random_seed;
extern char step_string[STRINGLENGTH];
extern char *Default_step_string;
extern int  step_size;
extern boolean  start_up;
