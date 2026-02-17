/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* display.c

	Update the screen based on the display template
	that we read in from the user.
	
	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/
/*LINTLIBRARY*/

#include "general.h"
#include "io.h"
#include "variable.h"
#include "template.h"
#include "weights.h"
#include "command.h"

/* the top level function in this module is update_display().

   This function parses the template list, and prints only those
   things that are turned on.  It prints things using the given
   display type, and tempered by the type of the variable.  This 
   means that there are a lot of small functions here that do  
   specific type and variable-type displaying.
   
   The main program should first call init_display() to set up the
   screen.  This mainly means that the screen is cleared.
   No other updating of the screen is done.
   
*/

int	num_lines = MAX_SCREEN_LINES;
int	num_cols = MAX_SCREEN_COLUMNS;
int     command_x = 0;		/* start points for the command line */
int     command_y = 0;		/* they default to (0,0) */
int     Display_level = 0;
int     Save_level = 0;
int     Screen_clear = 0;
int	stand_out = 1;		/* stand out mode defaults to on */
int	saveit = 0;
int	logflag = 0;

FILE	*lfp = NULL;			/* logfile */

/* initialize the display screen */
init_display() {
    int     set_log(),save_screen();

    install_command("screen",save_screen, SAVEMENU,(int *) NULL);
    install_var("standout", Int,(int *) & stand_out,0,0, 
    							DISPLAYOPTIONS);
    install_var("dlevel", Int,(int *) &Display_level, 0, 0,SETPCMENU);
    install_var("slevel", Int,(int *) &Save_level, 0, 0, SETPCMENU);
}

end_display() {
    io_endwin();
}

clear_display() {
    io_clear();
    Screen_clear = 1;
    io_refresh();
    return(CONTINUE);
}

update_display() {
    struct Template *tp;
    int tindex;
    int saved = 0;

/* if the screen is clear, we must begin by printing the background  */

    if(Screen_clear && layout_defined)
	display_background();

    tindex = ntemplates;
    
    while (--tindex >= 0) {	 /* list is backwards */
    	tp = torder[tindex];

	if ((tp->display_level > 0 && tp->display_level <= Display_level)
		|| (tp->display_level == Display_level)
		|| (Screen_clear && !tp->display_level)) {
	    if(logflag && tp->display_level>0 && 
	    				tp->display_level<= Save_level)
		 {
		 saved++;
		 saveit = 1;
		 }
	    update_template(tp);
	    saveit = 0;
	}
    }
    if(saved) {
    	fprintf(lfp,"\n");
	fflush(lfp);
    }
    io_refresh();
    Screen_clear = 0;
    return(CONTINUE);
}

redisplay() {
	clear_display();
	update_display();
}

update_template(tp)
struct Template *tp;
{
    if (tp->defined == 0) {
	if (!try_to_define(tp)) {
		sprintf(err_string,"Undefined variable in template file: %s.",
			tp->var);
		return(put_error(err_string));
	}
    }
    switch (tp->type) {
	case VECTOR: 
	    display_vector(tp);
	    break;
	case MATRIX: 
	    display_matrix(tp);
	    break;
	case VARIABLE:
	    display_variable(tp);
	    break;
	case LABEL: 
	    display_label(tp);
	    break;
	case LABEL_ARRAY: 
	    display_label_array(tp);
	    break;
	case LOOK: 
	    display_look(tp);
	    break;
	case LABEL_LOOK: 
	    display_label_look(tp);
	    break;
	case FLOATVAR: 
	    display_float_variable(tp);
	    break;
	default: 
	    return(put_error("Error: unknown display type in template\n"));
    }
    return(CONTINUE);
}

/* ARGSUSED */
do_update_template(str, intp)
char   *str;
int    *intp;
{
    struct Template *tp = (struct Template *) intp;

    return(update_template(tp));
}

try_to_define(template)
struct Template *template;
{
    struct Variable *var;

    if ((var = lookup_var((char *) template->var)) == NULL)
	return(FALSE);
    else {
	free((char *) template->var);
	template->var = var;
	template->defined = 1;
	return(TRUE);
    }
}


/* display a string in the given number of spaces with a given number of
   leading spaces     */

display_string(str, nchars, npads)
char   *str;
int     nchars;
int     npads;
{
    char    fmt[20];
    char    string[BUFSIZ];
    int	    i;
    int	    nc;

    *string = '\0';
    for(i=0; i < npads; i++)
       strcat(string," ");
    strcat(string,str);
    if (nchars >= 0) {
      (void) sprintf(fmt, "%%-%d.%ds", nchars, nchars);
    }
    else {
      nc = - nchars;
      (void) sprintf(fmt, "%%%d.%ds", nc, nc);
    }
    io_printw(fmt,string);
    if (saveit) fprintf(lfp," %s",str);
}


/* display an integer given a specific scale and number of digits */

display_integer(val, dig, scale)
int     val;
int     dig;
float   scale;
{
    long    lval;

    lval = (long)(val * scale);
    print_digits(lval, dig);
    if(saveit) fprintf(lfp," %d",val);
}

/* display a float given a specific scale and number of digits */

#define FUDGE 0.0000001

display_float(val, dig, scale)
float  val;
int     dig;
float  scale;
{
    float  tmp;
    long    lval;

    tmp = (val * scale);
    if (tmp >= 0) tmp += FUDGE;
    else tmp -= FUDGE;
    /* FUDGE is supposed to ensure that floats on the razor's edge
       between two integer values are not incorrectly truncated downward */
    lval = (long) tmp;
    print_digits(lval, dig);
    if(saveit) fprintf(lfp," %6.3f",val);
}

/* this function prints a single floating value as a float */

display_as_float(val, dig, scale)
float  val;
int     dig;
float  scale;
{
    char    fmt[10];
    char    tmp[100];

    val = val * scale;
    (void) sprintf(fmt, "%%%d.4f", dig);
    (void) sprintf(tmp,fmt,val);
    (void) sprintf(fmt, "%%%d.%ds",dig,dig);
    io_printw(fmt, tmp);
    if(saveit) fprintf(lfp," %6.3f",val);
}

/* finally, we get to the routines for displaying the various
   template types */

display_vector(template)
struct Template *template;
{
    float  *fptr;
    int    *iptr;
    char   **sptr;
    int     count, end, start, max_count;
    int     digits;
    boolean orientation;
    float   scale;

    if (template->var == NULL)
	return(put_error("Undefined Template Encountered."));

    io_move(template->y, template->x);
    start = template->min_x;
    max_count = start+(template->max_x);
    end = template->var->max_x;
    digits = template->digits;
    scale = template->precision;
    orientation=template->orientation;
    switch (template->var->type) {
	case Vint: 
	    iptr = (int *) template->var->varptr;
	    for (count = start; count < end && count < max_count; count++) {
		if(!orientation) {
			io_move(template->y+count-start,template->x);
		}
		display_integer(iptr[count], digits, scale);
	    }
	    break;
	case Vfloat: 
	    fptr = (float *) template->var->varptr;
	    for (count = start; count < end && count < max_count; count++) {
		if(!orientation) {
			io_move(template->y+count-start,template->x);
		}
		display_float(fptr[count], digits, scale);
	    }
	    break;
	case Vstring: 
	    sptr = (char **) template->var->varptr;
	    for (count = start; count < end && count < max_count; count++) {
		if(!orientation) {
			io_move(template->y+count-start,template->x);
		}
		display_string(sptr[count], digits,(int)scale);
	    }
	    break;
	default: 
	    sprintf(err_string,"Error: cannot display %s as vector.\n", 
						    template->var->name);
	    return(put_error(err_string));
    }
    return(CONTINUE);
}

display_matrix(template)
struct Template *template;
{
    float  *fptr;
    float  **pfptr;
    int     maxr,firstr,lastr,maxs,firsts,lasts,r,s,ts,tr;
    int     x,y;
    boolean orientation;
    int     digits;
    float   scale;
    int tempint;

    if (template->var == NULL)
	return(put_error("Undefined Template Encountered."));

    x = template->x;
    y = template->y;
    maxr = template->var->max_x; /* in the variable structure, x is
				    used to index rows, y columns! */
    firstr = template->min_x;
    lastr = firstr+template->max_x;
    firsts= template->min_y;
    maxs = template->var->max_y;
    lasts = firsts+template->max_y;
    digits = template->digits;
    scale = template->precision;
    orientation = template->orientation;
    pfptr = (float **) template->var->varptr;
    io_move(y, x);

    switch (template->var->type) {
	case PVweight: 
	    for (r = firstr,tr = 0; r < lastr && r < maxr; r++,tr++) {
		 maxs = first_weight_to[r] + num_weights_to[r];
	         for (s = firsts,ts = 0; s < lasts && s < maxs; s++,ts++) {
			if (s >= first_weight_to[r]) {
			  if (orientation) io_move(y+tr, x+(ts*digits));
			  else io_move(y+ts, x+(tr*digits));
			  tempint = s - first_weight_to[r];
		          display_float(pfptr[r][tempint], digits, scale);
			  /* tempint is a kludge for Microsoft C compiler */
			}
		}
	    }
	    break;
	case PVfloat: 
	    for (r = firstr,tr = 0; r < lastr && r < maxr; r++,tr++) {
	         for (s = firsts,ts = 0; s < lasts && s < maxs; s++,ts++) {
			if (orientation) io_move(y+tr, x+(ts*digits));
			else io_move(y+ts, x+(tr*digits));
		        display_float(pfptr[r][s], digits, scale);

		}
	    }
	    break;
	default: 
	    sprintf(err_string,"Error: cannot display %s in matrix form\n", 
						    template->var->name);
	    return(put_error(err_string));
    }
    return(CONTINUE);
}

display_variable(template)
struct Template *template;
{
    float  *fptr;
    int    *iptr;
    char   *cptr;
    char   *sptr;
    int     digits;
    float   scale;

    if (template->var == NULL)
	return(put_error("Undefined Template Encountered."));

    io_move(template->y, template->x);
    digits = template->digits;
    scale = template->precision;
    switch (template->var->type) {
	case Char: 
	    cptr = (char *) template->var->varptr;
	    io_printw("%c", *cptr);
	    break;
	case Int: 
	    iptr = (int *) template->var->varptr;
	    display_integer(*iptr, digits, scale);
	    break;
	case Float: 
	    fptr = (float *) template->var->varptr;
	    display_float(*fptr, digits, scale);
	    break;
	case String: 
	    sptr = (char *) template->var->varptr;
	    display_string(sptr, digits, 0);
	    break;
	default: 
	    sprintf(err_string,
	       "Error: cannot display %s as a single variable.\n",
				template->var->name);
	    return(put_error(err_string));
    }
    return(CONTINUE);
}

/* display a float as a float using standard printf formatting */

display_float_variable(template)
struct Template *template;
{
    float  *fptr;
    int     digits;
    float   scale;

    if (template->var == NULL)
	return(put_error("Undefined Template Encountered."));

    io_move(template->y, template->x);
    digits = template->digits;
    scale = template->precision;
    switch (template->var->type) {
	case Float: 
	    fptr = (float *) template->var->varptr;
	    display_as_float(*fptr, digits, scale);
	    break;
	default: 
	    sprintf(err_string,
	    	"Error: cannot display %s in floating point notation\n", 
				template->var->name);
	    return(put_error(err_string));
    }
    return(CONTINUE);
}


/* print the label */
display_label(template)
struct Template *template;
{
    print_string((char *) template->name, template->orientation, 
			template->x, template->y, template->digits);
}


/* print the label array:  this means running through the array, and 
   printing each label at the correct position.  if the orientation is
   horizontal, then we print horizontaly, and drop one line.  if the 
   orientation is vertical, then we print verticaly, and move over one
   character.
*/

display_label_array(template)
struct Template *template;
{
    int     count,
    	    start,
            end,
	    max_count;
    int     x,
            y;
    char  **strp;

    if (template->var == NULL)
	return(put_error("Undefined Template Encountered."));

    if (template->var->type != Vstring) {
    	sprintf(err_string,"Error: cannot display %s as label array.",
		template->var->name);
	return(put_error(err_string));
    }
		
    start = template->min_x;
    max_count = start+template->max_x;
    end = template->var->max_x;
    x = template->x;
    y = template->y;

    strp = (char **) template->var->varptr + start;

    if (template->orientation) {
	for (count = start; count < end && count < max_count; count++, y++) {
	    print_string(*strp++, template->orientation, x, y,
			 template->digits);
	}
    }
    else {
	for (count = start; count < end && count < max_count; count++, x++) {
	    print_string(*strp++, template->orientation, x, y, 
			 template->digits);
	}
    }
    return(CONTINUE);
}

display_look(template)
struct Template *template;
{
    float  *fptr;
    float  **pfptr;
    int    *iptr;
    char   **look;		/* pointer to the look array */
    int     endr,		/* last column in actual array */
            endc,		/* last row in actual array */
            max_x,		/* largest x index of look array */
            max_y;		/* largest y index of look array */
    int     offx,		/* offsets for the look */
            offy;
    int     y,			/* y index in the look */
            x;			/* x index in the look */
    int     digits;
    float   scale;
    int	    spacing;
    int	    type;
    int	    row, col;		/* indexes to array elements */
    int	    windex;		/* weight index */
    int	    index;

    if (template->var == NULL)
	return(put_error("Undefined Template Encountered."));

    look = template->look->look_template;
    offx = template->x;
    offy = template->y;
    endr = template->var->max_x; /* in the variable structure, 
				    x is used to index
				    rows, y to index columns! */
    endc = template->var->max_y;
    max_x = template->look->look_x * template->spacing;
    max_y = template->look->look_y;
    digits = template->digits;
    scale = template->precision;
    spacing = template->spacing;
    type = template->var->type;
    io_move(offy, offx);
    switch (template->var->type) {
	case Vint: 
	    iptr = (int *) template->var->varptr;
	    for (y = 0; y < max_y; y++) {
		for (x = 0; x < max_x; x += spacing) {
		    if ((*look != NOCELL) && ((index = atoi(*look)) < endr)) {
			io_move(offy + y, offx + x);
			display_integer(*(iptr + index), digits, scale);
		    }
		    look++;	/* go to the next look cell */
		}
	    }
	    break;
	case Vfloat: 
	    fptr = (float *) template->var->varptr;
	    for (y = 0; y < max_y; y++) {
		for (x = 0; x < max_x; x += spacing) {
		    if ((*look != NOCELL) && ((index = atoi(*look)) < endr)) {
			io_move(offy + y, offx + x);
			display_float(*(fptr + index), digits, scale);
		    }
		    look++;	/* go to the next look cell */
		}
	    }
	    break;
	case PVfloat:
	case PVweight:
	    pfptr = (float **) template->var->varptr;
	    for (y = 0; y < max_y; y++) {
		for (x = 0; x < max_x; x += spacing) {
		    if (*look != NOCELL) {
			io_move(offy + y, offx + x);
			sscanf(*look,"%d,%d",&row,&col);
			if (row < endr && col < endc) {
			  fptr = pfptr[row];
			  if (type == PVfloat) {
			    fptr += col;
			    display_float(*fptr, digits, scale);
			  }
			  else { /* its a PVweight, so take care of offsets */
			    windex = (col - first_weight_to[row]);
			    if (windex < 0 || windex < num_weights_to[row]) {
			        fptr += windex;
				display_float(*fptr, digits, scale);
			    }
			  }
			}
		    }
		    look++;
		}
	    }
	    break;
	default: 
	    sprintf(err_string,"Error: look cannot display %s.\n", 
	    			template->var->name);
	    return(put_error(err_string));
    }
    return(CONTINUE);
}

display_label_look(template)
struct Template *template;
{
    char  **look;		/* pointer to the look array */
    char  **sptr;		/* pointer to list of labels */
    int     end,		/* last index in actual array */
            max_x,		/* largest x index of look array */
            max_y;		/* largest y index of look array */
    int     offx,		/* offsets for the look */
            offy;
    int     inc_y, inc_x;	/* increments to y and x */
    int     y,			/* y index in the look */
            x;			/* x index in the look */
    int     digits;
    boolean orientation;
    int     spacing;
    int	    index;

    if (template->var == NULL)
	return(put_error("Undefined Template Encountered."));

    if (template->var->type != Vstring) {
    	sprintf(err_string,"Error: cannot display %s as label array.",
		template->var->name);
	return(put_error(err_string));
    }
		
    look = template->look->look_template;
    offx = template->x;
    offy = template->y;
    end = template->var->max_x;
    digits = template->digits;
    orientation = template->orientation;
    spacing = template->spacing;
    if (orientation == HORIZONTAL) {
      max_x = template->look->look_x * spacing;
      max_y = template->look->look_y;
      inc_y = 1;
      inc_x = spacing;
    }
    else {
      max_x = template->look->look_x;
      max_y = template->look->look_y * spacing;
      inc_x = 1;
      inc_y = spacing;
    }
    sptr = (char **) template->var->varptr;
    for (y = 0; y < max_y; y += inc_y) {
	for (x = 0; x < max_x; x += inc_x) {
	    if ((*look != NOCELL) && ((index = atoi(*look)) < end)) {
	        print_string(*(sptr + index), orientation, x+offx, y+offy,
			     			digits);
	    }
	    look++;	/* go to the next look cell */
	}
    }
    return(CONTINUE);
}

/* this procedure is the workhorse for printing numeric values.
   it is optimized for printing digit lengths of 1,2, or 3. But
   it will work for digit lengths up to 10.  It prints the number
   in the correct number of digits, if the number fits.  otherwise
   it prints stars.  A number that is less than 0, is printed in
   reverse video.						*/

print_digits(lval, dig)
long    lval;
int     dig;
{
    char    fmt[20];
    long    temp;
    double  pow ();

    if (lval < 0) {
      if (stand_out) {
	io_standout();
      }
      else {
	print_neg_digits(lval, dig);
	return;
      }
    }
    temp = ((lval < 0) ? -lval : lval);
    if (dig == 1) {
	if (temp > 9) {
	    io_printw("%1s", "*");
	}
	else {
	    io_printw("%1ld", temp);
	}
    }
    else
	if (dig == 2) {
	    if (temp > 99) {
		io_printw("%2s", "**");
	    }
	    else {
		io_printw("%2ld", temp);
	    }
	}
    else
	if (dig == 3) {
	    if (temp > 999) {
		io_printw("%3s", "***");
	    }
	    else {
		io_printw("%3ld", temp);
	    }
	}
    else {
	if ((double) temp > (double)(pow(10.0,(double) dig) - 1.0)) {
	    (void) sprintf(fmt, "%%%d.%ds", dig, dig);
	    io_printw(fmt, "***********");
	}
	else {
	    (void) sprintf(fmt, "%%%dld", dig);
	    io_printw(fmt, temp);
	}
    }
    if (lval < 0)
	io_standend();
}

/* this procedure is used to print strings.

   It is special, in that it will print strings vertically as
   well as the usual horizontal.
*/
print_string(str, horizontal, x, y, digits)
char   *str;
boolean horizontal;
int     x,
        y;
int     digits;			/* number of characters to display */
{
    int     count,
            end;
    char    fmt[20];

    if (horizontal) {
	io_move(y, x);
	(void) sprintf(fmt, "%%-%d.%ds", digits, digits);
	io_printw(fmt, str);
    }
    else {
	end = strlen(str);
	for (count = 0; count < end && count < digits; count++, str++) {
	    io_move(y++, x);
	    io_printw("%c", *str);
	}
    }
}


display_background()
{
   int	x,y;

   for(x = 0; x <= num_cols;x++)
       for(y = 0; y <= num_lines-5;y++)
   	  if( background[y][x] != '\0') {
   		io_move(y+5,x);
   		io_printw("%c",background[y][x]);
   	}
}
	   
/* this is called from the command system.  it allows change of
   selected parts of the display.  Essentially, you can turn the display
   on or off, and change the numbers of digits, or the scaling factor.
   */
/* ARGSUSED */
change_display(command_string, tempp)
char   *command_string;
int    *tempp;
{
    struct Template *tp;
    char    command1[BUFSIZ];
    char   *str;

    tp = (struct Template  *) tempp;

    (void) sprintf(command1,
	    "%s: change what  [level = %d, #digits = %d, scale = %.3f]",
	    tp->name, tp->display_level, tp->digits, tp->precision);

    while ((str = get_command(command1)) != NULL) {
	if (startsame(str, "level")) {
	    (void) strcat(command1, " level:");
	    if ((str = get_command(command1)) != NULL) {
		if (sscanf(str,"%d",&(tp->display_level)) == 0) {
			return(put_error("Invalid disp level."));
		}
	    }
	}
	else
	    if (startsame(str, "digits") || startsame(str,"#digits")) {
		(void) strcat(command1, " digits:");
		if ((str = get_command(command1)) != NULL) {
		    if (sscanf(str,"%d",&(tp->digits)) == 0) {
		    	return(put_error("Invalid digits."));
		    }
		}
	    }
	else
	    if (startsame(str, "scale")) {
		(void) strcat(command1, " scale:");
		if ((str = get_command(command1)) != NULL) {
		    if (sscanf(str,"%f",&(tp->precision)) == 0) {
		    	return(put_error("Invalid scale."));
		    }
		}
	    }
	else {
		return(put_error("Unrecognized display option."));
	}
	return(CONTINUE);
    }
}
/* used to print negative digits when standout mode is not set */

print_neg_digits(lval, dig)
long    lval;
int     dig;
{
    char    fmt[20];
    char    dstr[10];
    long    temp;
    double  pow ();
    char    *dlets = "oabcdefghiX";

    temp = (-lval);
    if (dig == 1) {
	if (temp > 9) {
	    io_printw("%1s", "X");
	}
	else {
	    io_printw("%c", dlets[temp]);
	}
    }
    else
	if (dig == 2) {
	    if (temp < 10) {
	        io_printw("%2ld", lval);
	    }
	    else if (temp > 99) {
		io_printw("%2s", "XX");
	    }
	    else {
		(void) sprintf(dstr,"%c%c", dlets[temp/10],dlets[temp%10]);
		io_printw("%2s",dstr);
	    }
	}
    else
	if (dig == 3) {
	    if (temp < 100) {
	        io_printw("%3ld",lval);
	    }
	    else if (temp > 999) {
		io_printw("%3s", "XXX");
	    }
	    else {
		(void) sprintf(dstr,
		    "%c%c%c", dlets[temp/100],dlets[(temp/10)%10],dlets[temp%10]);
		io_printw("%3s",dstr);
	    }
	}
    else {
	if ((double) temp > (double)(pow(10.0,(double) dig-1) - 1.0)) {
	    (void) sprintf(fmt, "%%%d.%ds", dig, dig);
	    io_printw(fmt, "XXXXXXXXXXX");
	}
	else {
	    (void) sprintf(fmt, "%%%dld", dig);
	    io_printw(fmt, lval);
	}
    }
}

set_log() {
    char   *str;
    char string[BUFSIZ];

    str = get_command("file name (- to close log): ");
    if (str == NULL ) {
        return(put_error("no change made in logging status"));
    }

    if(logflag) fclose(lfp);

    logflag = 0;
    
    if (*str == '-') {
	return(CONTINUE);
    }

    if ((lfp = fopen(str, "a")) == NULL) {
	return(put_error(
	       "cannot open file for output -- logging not enabled"));
    }
    logflag = 1;
    return(CONTINUE);
}

save_screen() {
    FILE *sfp;
    char *str;
    int c,l,rval;
    char ch;
    char sv_stout = 0;
    
    rval = CONTINUE;
    
    if (stand_out) sv_stout = 1;
    
    if (sv_stout) {
    	stand_out = 0;
    	update_display();
    }    

    str = get_command("file name (return to abort): ");
    if (str == NULL) goto finish_svscr;
    
    if ((sfp = fopen(str, "a")) == NULL) {
	rval = put_error("cannot open file for screen dump");
	goto finish_svscr;
    }

    for (l = 0; l < num_lines; l++) {
        for(c = 0; c < num_cols; c++) {
	   io_move(l,c);
	   ch = io_inch();
	   if (!ch) ch = ' ';
	   putc(ch,sfp);
	}
	putc('\n',sfp);
    }
    fclose(sfp);
    
finish_svscr:
    if (sv_stout) {
       stand_out = 1;
       clear_display();
       update_display();
    }
    return(rval);
}
