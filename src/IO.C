/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* Low Level io


	Generic low level I/O based on curses.
	Use only on an IBM/PC/XT/AT.
	On a real machine, use the curses library instead.

	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

/* the following commands are implemented from the standard
   unix curses library:
   
   io_initscr()	initialize the terminal routines.  Must be called
   		before any other screen i/o is used.

   io_endwin()	reset the terminal to the state it was in before
   		calling io_initscr().

   io_clear()	io_clear the screen.
   
   io_clrtoeol()	io_clear from the current position to the end of line.
   
   io_move(line,col)	io_move the current position to line,col.
   
   io_standout()	do all further printing in reverse video.
   
   io_standend()	do all further printing in standard video.
   
   io_printw(fmt,arg)	    print at the current screen position
   				using standard printf formating.
				These must always be a format and a 
				single argument.
   io_printw modified 6-Apr-87 by MAF: must be io_printw(fmt, arg1);
   (i.e., io_printw must have exactly two arguments)

   All of these commands are defined by printing the required screen
   commands directly to the PC screen.  For a different PC, the only
   changes needed should be to the #define's of the screen commands.
*/

#include <stdio.h>
#include "io.h"
#ifdef MSDOS
char	xxtemp[200];	/* added 6-Apr-87 by MAF; used by newprint(), below */
int	io_type = IO_NORMAL;
#endif

extern FILE *in_stream;
extern int start_up;
#ifdef CURSES

#include <curses.h>

io_initscr() {
    initscr();
    crmode();
    noecho();
}

io_endwin() {
    if (start_up) return;
    endwin();
    nocrmode();
    echo();
}

io_addch(c) char c; {
    addch(c);
}

io_clear() {
    if (start_up) return;
    clear();
}

io_clrtoeol() {
    if (start_up) return;
    clrtoeol();
}

io_move(line, col) {
    if (start_up) return;
    move(line, col);
}

io_standout() {
    if (start_up) return;
    standout();
}

io_standend() {
    if (start_up) return;
    standend();
}

io_refresh() {
    if (start_up) return;
    refresh();
}

io_inch() {
    if (start_up) return;
    return(inch());
}

#else  (if not CURSES)

#ifdef MSDOS
#include <dos.h>
#define VIDEO_INTERRUPT	0x10
#define PAGE		0
#define SET_POSITION	2
#define READ_POSITION	3
#define WRITE_CHAR	9
#define READ_CHAR	8
#define SCROLL_UP	6
#endif

io_initscr() {
}

io_endwin() {
}

io_clear() {
#ifdef MSDOS
    union REGS vregs;
    
    if (start_up) return;
    
    vregs.h.ah = SCROLL_UP;
    vregs.h.al = 0; /* clear screen */
    vregs.h.ch = 0;
    vregs.h.cl = 0;
    vregs.h.dh = 23;
    vregs.h.dl = 79;
    vregs.h.bh = IO_NORMAL;
    
    int86(VIDEO_INTERRUPT, &vregs, &vregs);

#else (if not MSDOS)

    if (start_up) return;
    print(CLEARSCREEN);

#endif MSDOS        
}

io_clrtoeol() {  /* clear to end of line */
#ifdef MSDOS
    static union REGS inregs1, outregs1, inregs2, outregs2;
    static int row,col;
    
    if (start_up) return;
    
    /* first get current cursor position */
    inregs2.h.ah = READ_POSITION;
    inregs2.h.bh = PAGE;
    int86(VIDEO_INTERRUPT, &inregs2, &outregs2);
    row = outregs2.h.dh;
    col = outregs2.h.dl;
    
    /* now set up inregs1 for the interrupt to output a space */
    inregs1.h.ah = WRITE_CHAR;
    inregs1.h.bh = PAGE;
    inregs1.x.cx = 1;
    inregs1.h.bl = IO_NORMAL;
    inregs1.h.al = ' ';
    
    /* now set up inregs2 for the interrupt to move cursor right */
    inregs2.h.ah = SET_POSITION;
    inregs2.h.dh = row;
    while (col < 80) {
        /* write a blank space at column number col */
	int86(VIDEO_INTERRUPT, &inregs1, &outregs1);
	
	/* move cursor right */
	inregs2.h.dl = (++ col);
	int86(VIDEO_INTERRUPT, &inregs2, &outregs2);
    }

#else (if not MSDOS)

    if (start_up) return;
    print(CLEARTOEOL);
    
#endif MSDOS
}


io_move(line, col) {

#ifdef MSDOS
    /* cprintf("%s%d;%dH", MOVECURSOR, line + 1, col + 1);
    Previous version used the above line for cursor movement.
    This version uses a software interrupt, below.  -- 10-Mar-87 MAF */

    union REGS vregs;

    if (start_up) return;

    vregs.h.ah = SET_POSITION;
    vregs.h.dh = line;
    vregs.h.dl = col;
    vregs.h.bh = PAGE;
    
    int86(VIDEO_INTERRUPT, &vregs, &vregs);
#else
    if (start_up) return;
    printf("%s%c%c", MOVECURSOR,(char)(line + 32),(char)(col + 32));
#endif MSDOS
}


io_standout() {
    if (start_up) return;
#ifdef MSDOS
    io_type = IO_REVERSE; /* 6-Apr-87 MAF */
#else if not MSDOS
    print(STANDOUT);
#endif
}

io_standend() {
    if (start_up) return;
#ifdef MSDOS
    io_type = IO_NORMAL; /* 6-Apr-87 MAF */
#else if not MSDOS    
    print(STANDEND);
#endif MSDOS    
}


#ifdef MSDOS
newprint(xx)
char xx[];
{
    static int i, row, column;
    extern int io_type;
    static union REGS inregs1, outregs1, inregs2, outregs2;
    
    inregs1.h.ah = WRITE_CHAR;
    inregs1.h.bh = PAGE;
    inregs1.x.cx = 1; /* count of characters to write */
    inregs1.h.bl = io_type; /* char attribute */
    
    inregs2.h.ah = READ_POSITION;
    inregs2.h.bh = PAGE;
    int86(VIDEO_INTERRUPT, &inregs2, &outregs2); /* read row & column */
    row = outregs2.h.dh;
    column = outregs2.h.dl;
    
    inregs2.h.ah = SET_POSITION;
    inregs2.h.dh = row;
    
    i = -1;
    while (xx[++i] != 0) {
        inregs1.h.al = xx[i];
	int86(VIDEO_INTERRUPT, &inregs1, &outregs1); /* print character */
	
	inregs2.h.dl = ++column;
	int86(VIDEO_INTERRUPT, &inregs2, &outregs2); /* move cursor right */
    }
}
#endif MSDOS

io_refresh() {
    if (start_up) return;
    fflush(stdout);
}


io_inch() {
    
#ifdef MSDOS
    union REGS vregs;
    
    if (start_up) return;
    
    vregs.h.bh = PAGE;
    vregs.h.ah = READ_CHAR;
    
    int86(VIDEO_INTERRUPT, &vregs, &vregs);
    
    return( (char) vregs.h.al );
#else
    if (start_up) return;
    return('\0');
#endif
}

#endif  not CURSES

