/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* io.h

	Header file for curses-style io on PC

	First version implemented by Elliot Jaffe.
	
	Modified: Apr 6 1987;   maf; changes io_printw to use newprint()
	
	Date of last revision:  8-12-87/JLM.
*/

#ifdef MSDOS
#define print(x)	printf("%s",x)
#define io_printw(f,a)	if (!start_up) {sprintf(xxtemp,f,a); newprint(xxtemp);}

#define IO_REVERSE	0x70	/* io_type == 0x70 for reverse video */
#define IO_NORMAL	0x07	/* io_type == 0x07 for normal video */
/* Above two lines added 6-Apr-87 MAF */

#define	CLEARSCREEN	"\033[2J"/* <ESC>[2J	 */
#define MOVECURSOR	"\033["	/* <ESC>[%+;%+f	 */
#define CLEARTOEOL	"\033[K"/* <ESC>[K	 */
#define STANDOUT	"\033[7m"/* <ESC>[7m	 */
#define STANDEND	"\033[0m"/* <ESC>[0m	 */
#else
#ifdef h19
#define print(x)	printf(x)
#define io_printw(f,a)	if (!start_up) printf(f,a)
#define	CLEARSCREEN	"\033E"	/* <ESC>E	 */
#define MOVECURSOR	"\033Y"	/* <ESC>Y%+ %+	 */
#define CLEARTOEOL	"\033K"	/* <ESC>K	 */
#define STANDOUT	"\033p"	/* <ESC>p	 */
#define STANDEND	"\033q"	/* <ESC>q	 */
#else
#define CURSES 1
#define io_printw(f,a)	if (!start_up) printw(f,a)
#endif
#endif

