/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* template.c

	File for reading and scanning screen templates.

	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

#include "general.h"
#include "command.h"
#include "variable.h"
#include "display.h"
#include "template.h"

struct Template *tlist = 0;	/* template list: linked list */
struct Template **torder;	/* list of pointers to templates */
int ntemplates = 0;		/* number of templates */

boolean	layout_defined = 0;

char	**background;
int	num_slots = 0;

struct slot_locations {
	int	xloc;
	int	yloc;
};

struct slot_locations *slot_loc;
int    maxslots = MAXSLOTS;

int template_level;
int template_x;
int template_y;

/* install a template with the given name, at the given position (x,y)
   Defaulting the display to be on/off, with the variable var. */
   
struct Template *
install_template (s, type, display_level, varname, 
            x, y, orient,min_x,min_y,max_x, max_y, digits, precision,spacing)
char   *s;			/* this is the users name for the displays */
int     type;			/* the type of display */
int     display_level;
char	*varname;
int     x, y;			/* starting (x,y) position on the screen */
boolean orient;			/* on for horizonal off for vertical */
int     min_x, min_y, max_x, max_y, digits;
float   precision;
int     spacing;
{
    struct Template *sp;
    char   *emalloc (), *strcpy ();
    struct Variable *var, *lookup_var ();
    
    sp = (struct Template  *) emalloc (sizeof (struct Template));
    sp->name = emalloc((unsigned)(strlen(s) + 1));
    (void) strcpy(sp->name, s);

    if (type != LABEL) {
      if (varname != (char *) NULL) {
	var = lookup_var(varname);
      }
      if (var == NULL) {
	sp->defined = 0;
	sp->var = (struct Variable *) 
			emalloc ((unsigned)(strlen (varname) + 1));
	(void) strcpy((char *) sp->var, varname);
      }
      else {
	sp->defined = 1;
	sp->var = var;
      }
      install_command(sp->name, change_display, DISPLAYOPTIONS,
    								(int *) sp);
      install_command(sp->name, do_update_template, DISPLAYMENU,
    								(int *) sp);
    }
    else {
        sp->var = NULL;
	sp->defined = -1;
    }
    
    sp->type = type;
    sp->display_level = display_level;
    sp->x = x;
    sp->y = y;
    sp->orientation = orient;
    sp->min_x = min_x;
    sp->min_y = min_y;
    sp->max_x = max_x;
    sp->max_y = max_y;
    sp->digits = digits;
    sp->precision = precision;
    sp->spacing = spacing;
    sp->look = NULL;
    sp->next = tlist;
    tlist = sp;
    return sp;
}

read_template() {
    char    name[40];
    char    type[40];

    while (fscanf(in_stream, "%s %s", name, type) != EOF) {
	if(strcmp("layout", type) == 0) {
	    read_background();
	}
	else
	    if (strcmp("vector", type) == 0) {
		read_vector(name);
	    }
	else
	    if (strcmp("matrix", type) == 0) {
		read_matrix(name);
	    }
	else
	    if (strcmp("label", type) == 0) {
		read_label(name);
	    }
	else
	    if (strcmp("label_array", type) == 0) {
		read_label_array(name);
	    }
	else
	    if (strcmp("variable", type) == 0) {
		read_variable(name);
	    }
	else
	    if (strcmp("look", type) == 0) {
		read_look(name);
	    }
	else
	    if (strcmp("label_look", type) == 0) {
		read_label_look(name);
	    }
	else
	    if (strcmp("floatvar", type) == 0) {
		read_float_variable(name);
	    }
	else 
	    return(put_error("Undefined template type encountered."));
    }
    make_torder();
    return(CONTINUE);
}


read_vector(name)
char   *name;
{
    char    Varname[40];
    int     digits;
    float   precision;
    char    Orient[2];
    boolean orientation;
    int     start,stop,level;

    get_template_xy();

    (void) fscanf(in_stream, "%s%s%d%f%d%d",
		  Varname,Orient, &digits, &precision,&start, &stop);
    
    if (strcmp(Orient, "h") == 0) {
	orientation = TRUE;
    }
    else {
	orientation = FALSE;
    }

    install_template(name, VECTOR, template_level, Varname, template_x,
	    template_y,orientation,start,0,stop, 0, digits, precision,0);

}

read_matrix(name)
char   *name;
{
    char    Varname[40];
    int     digits;
    float   precision;
    char    Orient[2];
    int	    orientation;
    int     first_row,num_rows,first_col,num_cols;

    get_template_xy();
    
    (void) fscanf(in_stream, "%s%s%d%f%d%d%d%d",
	    Varname, Orient, &digits, &precision,&first_row,&num_rows,
            &first_col, &num_cols);

    if (strcmp(Orient, "h") == 0) {
	orientation = TRUE;
    }
    else {
	orientation = FALSE;
    }

    install_template(name, MATRIX, template_level, Varname, template_x,
	    template_y,orientation,
	    first_row,first_col,num_rows,num_cols,digits, precision,0);
}

read_label(name)
char   *name;
{
    int     digits;
    char    Orient[2];
    char    str[40];
    boolean orientation;

    get_template_xy();
    
    (void) fscanf(in_stream, "%s%d", Orient, &digits);

    if (strcmp(Orient, "h") == 0) {
	orientation = TRUE;
    }
    else {
	orientation = FALSE;
    }

    install_template(name, LABEL, NULL, template_level, 
	    template_x, template_y, orientation,0,0,0, 0, digits, 0.0,0);
}

read_label_array(name)
char   *name;
{
    int     digits;
    char    varname[40];
    char    Orient[2];
    boolean orientation;
    int	    start,stop;

    get_template_xy();
    
    (void) fscanf(in_stream, "%s%s%d%d%d", 
		    varname, Orient, &digits,&start, &stop);

    if (strcmp(Orient, "h") == 0) {
	orientation = TRUE;
    }
    else {
	orientation = FALSE;
    }

    install_template(name, LABEL_ARRAY, template_level, varname, 
	 template_x, template_y, orientation,start,0,stop, 0, digits, 0.0,0);

}

read_variable(name)
char   *name;
{
    char    varname[40];
    char    str[40];
    int     dig;
    float   scale;

    get_template_xy();
    
    (void) fscanf(in_stream, "%s%d%f",varname, &dig, &scale);

    install_template(name, VARIABLE, template_level, varname, 
	    template_x, template_y, TRUE,0,0, 0, 0, dig, scale,0);
}

read_float_variable(name)
char   *name;
{
    char    varname[40];
    int     dig;
    float   scale;

    get_template_xy();
    
    (void) fscanf(in_stream, "%s%d%f", varname, &dig, &scale);

    install_template(name, FLOATVAR, template_level, varname, 
	    template_x, template_y, TRUE, 0,0,0, 0, dig, scale,0);
}


read_look(name)
char   *name;
{
    char    varname[40];
    char    filename[40];
    int     dig;
    float   scale;
    int     spacing;
    struct Template *look_temp;

    get_template_xy();
    
    (void) fscanf(in_stream, "%s%d%f%d%s",
	    varname, &dig, &scale, &spacing, filename);

    look_temp = install_template(name, LOOK, template_level, varname, 
	    template_x, template_y,TRUE, 0,0,0, 0, dig, scale, spacing);

    get_look(look_temp, filename);
}


read_label_look(name)
char   *name;
{
    char    varname[40];
    char    filename[40];
    int     dig;
    char    orient[2];
    int     spacing;
    boolean orientation;

    struct Template *look_temp;

    get_template_xy();
    
    (void) fscanf(in_stream, "%s%s%d%d%s",
	    varname, orient, &dig, &spacing, filename);
    
    if (strcmp(orient, "h") == 0) {
	orientation = HORIZONTAL;
    }
    else {
	orientation = VERTICAL;
    }

    look_temp = install_template(name, LABEL_LOOK, template_level, varname, 
	    template_x, template_y,orientation, 0,0,0, 0, dig, 0.0, spacing);

    get_look(look_temp, filename);
}


dump_template() {
    struct Template *tp;
    char    Type_Name[40];

    printf("TEMPLATES\n");
    printf("name    type    Lev  varname  x   y   orient  max_x   max_y  dig  scale\n");
    for (tp = tlist; tp != (struct Template *) NULL; tp = tp->next) {
	switch (tp->type) {
	    case COMMAND: 
		(void) strcpy(Type_Name, "command");
		break;
	    case VECTOR: 
		(void) strcpy(Type_Name, "vector");
		break;
	    case MATRIX: 
		(void) strcpy(Type_Name, "matrix");
		break;
	    case VARIABLE: 
		(void) strcpy(Type_Name, "variable");
		break;
	    case LABEL: 
		(void) strcpy(Type_Name, "label");
		break;
	    case LABEL_ARRAY: 
		(void) strcpy(Type_Name, "label_array");
		break;
	    case LOOK: 
		(void) strcpy(Type_Name, "look");
		break;
	    case LABEL_LOOK: 
		(void) strcpy(Type_Name, "label_look");
		break;
	    case FLOATVAR: 
		(void) strcpy(Type_Name, "floatvar");
		break;
	    default: 
		(void) strcpy(Type_Name, "unknown");
		break;
	}

	if (tp->defined == 1) {
	  printf("%-8.7s%-8.7s%-5d%-9.8s%-4d%-4d%-8d%-8d%-7d%-5d%-7.2f%-5d\n", 
		   tp->name, Type_Name, tp->display_level, tp->var->name, 
		   tp->x, tp->y, tp->orientation, tp->max_x, tp->max_y, 
		   tp->digits, tp->precision,tp->spacing);
	}
	else {
	  printf("%-8.7s%-8.7s%-5d%-9.8s%-4d%-4d%-8d%-8d%-7d%-5d%-7.2f%-5d\n", 
		 tp->name, Type_Name, tp->display_level, tp->name, 
		 tp->x, tp->y, tp->orientation, tp->max_x, tp->max_y, 
		 tp->digits, tp->precision,tp->spacing);
	}
    }
}

get_look(template, filename)
struct Template *template;
char   *filename;
{
    FILE * fp = NULL;
    char    string[BUFSIZ];
    struct Look *look;
    char     **look_table;

    if ((fp = fopen(filename, "r")) == NULL) {
    	sprintf(string,"Cannot open look file %s.",filename);
	return(put_error(string));
    }

    look = (struct Look *) emalloc (sizeof (struct Look));
    template->look = look;

 /* read in the maximum y and x coordinates of the look table */
    (void) fscanf(fp, "%d%d", &look->look_y, &look->look_x);

    look->look_template = ((char **) emalloc((unsigned)(sizeof(char *)
		* look->look_x * look->look_y)));
    look_table = look->look_template;

    while (fscanf(fp, "%s", string) != EOF) {
	if (*string == '.') {
	    *look_table = NOCELL;
	    look_table++;
	}
	else {
	    *look_table = ((char *) emalloc((unsigned)strlen(string)+1));
	    strcpy(*look_table,string);
	    look_table++;
	}
    }
    fclose(fp);
}

read_background()
{
	int x,y,i,back_lines;
	char string[BUFSIZ];
	fgets(string,BUFSIZ,in_stream);
	sscanf(string,"%d%d",&num_lines,&num_cols);
	if (num_lines > 5) back_lines = num_lines -5;
	else back_lines = 0;
	
	if (!back_lines) {
	    fgets(string,BUFSIZ,in_stream);
	    if(startsame("end",string)) 
	      return(CONTINUE);
	    else 
	      return(put_error("No lines available for background."));
	}
	slot_loc = ((struct slot_locations *) 
		    emalloc((unsigned)maxslots*sizeof(struct slot_locations)));
	background = ((char **) 
	             emalloc((unsigned)((back_lines+1)*sizeof(char *))));

	for(y = 0; y <= back_lines; y++) {
	   background[y] = 
		( (char *) emalloc ((unsigned)((num_cols+1)*sizeof(char))));
	   for(x = 0; x <= num_cols;x++) {
	      background[y][x] = '\0';
	   }
	}

	layout_defined++;
	num_slots = 0;
	y = 0;

	while(fgets(string,BUFSIZ,in_stream) != NULL) {
	   if(startsame("end",string))
	      break;
	   if(y == back_lines) {
		return(put_error(
		       "Background specification has too many lines."));
	   }
	   for(i = 0; i < num_cols; i++) {
	      if(string[i] == '\0') break;
	      if( (string[i] == ' ') || (string[i] == '\n') ) continue;
	      if( string[i] == '	') {
		 return(put_error(
			"no tabs allowed in background specification"));
	      }
	      if(string[i] == '$') {
	        if (num_slots >= maxslots) {
		    maxslots += 20;
		    slot_loc = ((struct slot_locations *) 
		       erealloc((char *)slot_loc, 
		        (unsigned)((maxslots-20)*sizeof(struct slot_locations)),
			(unsigned)maxslots*sizeof(struct slot_locations)));
		}
		slot_loc[num_slots].xloc = i;
		slot_loc[num_slots++].yloc = y+5;
	      }
	      else
		 background[y][i] = string[i];
	   }
	   y++;
	}
	return(CONTINUE);
}

make_torder() {
    struct Template *tp;
    
    ntemplates = 0;
    
    for (tp = tlist; tp != NULL; tp = tp->next) {
	ntemplates++;
    }
    torder = ( (struct Template **) emalloc 
	((unsigned) ntemplates * sizeof (struct Template *)));
	
    ntemplates = 0;
    for (tp = tlist; tp != NULL; tp = tp->next) {
    	torder[ntemplates++] = tp;
    }
}

int prev_slot = -1;

get_template_xy() {
    int slot;
    char ty_str[40], tx_str[40];
    
    fscanf(in_stream, "%d%s%s", &template_level, ty_str, tx_str);   
    
    if( ty_str[0] == '$') {
       if( tx_str[0] == 'n' ) slot = ++prev_slot;
       else prev_slot = slot = atoi(tx_str);
       if( slot >= num_slots) {
	 return(put_error("not enough dollar signs in background"));
       }
       template_y = slot_loc[slot].yloc;
       template_x = slot_loc[slot].xloc;
    }
    else {
      template_y = atoi(ty_str);
      template_x = atoi(tx_str);
    }
    return(CONTINUE);
}
