/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* variable.c

    This module implements a way to keep track of global variables
    in a C program and to access them interactively from their names.
   
    Note: this is essentialy just a symbol table.
   
    First version implemented by Elliot Jaffe.
	
    Date of last revision:  8-12-87/JLM.
*/

/*LINTLIBRARY*/

#include "general.h"
#include "variable.h"
#include "command.h"
#include "patterns.h"
#include "weights.h"

char **uname = NULL;
int  nunames;

int    *first_weight_to;  /* these guys are used for variables to type PVweight */
int    *num_weights_to;   /* they are generally installed in weights.c; 
			     arrays of type PVweight use them, but in aa,
			     weights are declared as PVfloat, so do not
			     use these */

static struct Variable *varlist = 0;/* variable table: linked list */

struct Variable *lookup_var (s)	/* find s in the variable list */
char   *s;
{
    struct Variable *sp;

    for (sp = varlist; sp != (struct Variable  *) NULL; sp = sp->next) {
	if (strcmp(sp->name, s) == 0)
	    return sp;
    }
    return NULL;		/* 0 ==> not found */
}
/* we replace this:
struct Variable *
   with void to avoid the hassle of casting to void on each call 
   abh - 2/15/88
*/
void install_var (s, t, varptr, max_x, max_y, menutype)
char   *s;
int     t;
int    *varptr;
int     max_x,
        max_y;  /* This represents the second dimension in matrices and */
		/* the offset of the unit name in vectors  */
int     menutype;
{
    struct Variable *sp;
    char   *emalloc (), *strcpy ();
    int     change_variable ();

    sp = (struct Variable  *) emalloc (sizeof (struct Variable));
    sp->name = emalloc((unsigned)(strlen(s) + 1));
 /* +1 for trailing NULL */
    (void) strcpy(sp->name, s);
    sp->type = t;
    sp->max_x = max_x;
    sp->max_y = max_y;
    sp->varptr = varptr;
    sp->next = varlist;

    if (menutype != NOMENU) {
	install_command(sp->name,change_variable,menutype,(int *)sp);
    }

    varlist = sp;
/*
    return sp;
*/
}


/* this function changes the max_x and max_y parameters of an 
   installed variable.  It returns TRUE if it succeeded, and
   FALSE otherwise.
*/

change_variable_length(str,x,y)
char *str;
int x,y;
{
    struct Variable *vp;
    
    vp = lookup_var(str);
    if(vp == NULL) return(FALSE);
    vp->max_x = x;
    vp->max_y = y;
    return(TRUE);
}    

/* this section of code changes all types of variables */
# define WEIGHTS 1
/* ARGSUSED */
change_variable(str, var)
char   *str;
int    *var;
{
    struct Variable *vp;

    vp = (struct Variable  *) var;

    switch (vp->type) {
	case Int: 
	    return(change_int_var(vp));
	case Float: 
	    return(change_float_var(vp));
	case String: 
	    return(change_string_var(vp));
	case Vint: 
	    return(change_ivector_var(vp));
	case Vfloat: 
	    return(change_fvector_var(vp));
	case Vstring: 
	    return(change_svector_var(vp));
	case PVfloat:
	    return(change_pfvector_var(vp,!WEIGHTS));
	case PVweight:
	    return(change_pfvector_var(vp,WEIGHTS));
	default: 
	    sprintf(err_string,"I don't know how to change %s", vp->name);
	    return(put_error(err_string));
    }
}


change_int_var(vp)
struct Variable *vp;
{
    char    string[40];
    char   *str;

    (void) sprintf(string, "%s = %d, new value: ", vp->name, *vp->varptr);

    str = get_command(string);
    if (str != NULL) {
	if (!sscanf(str,"%d",vp->varptr)) {
		return(var_error(vp->name,-1,-1));
	}
    }
    return(CONTINUE);
}


change_float_var(vp)
struct Variable *vp;
{
    char    string[40];
    char   *str;
    float  *fp;

    fp = (float *) vp->varptr;
    (void) sprintf(string, "%s = %f, new value: ", vp->name, *fp);

    str = get_command(string);
    if (str != NULL) {
	if (!sscanf(str,"%f",fp)) {
		return(var_error(vp->name,-1,-1));
	}
    }
    return(CONTINUE);
}

change_string_var(vp)
struct Variable *vp;
{
    char    string[40];
    char   *str;

    str = (char *) vp->varptr;
    (void) sprintf(string, "%s = %s, new value: ", vp->name, str);

    str = get_command(string);
    if (str != NULL) {
	(void) strcpy((char *) vp->varptr, str);
    }
    return(CONTINUE);
}


change_ivector_var(vp)
struct Variable *vp;
{
    char    string[BUFSIZ];
    char    tempstring[BUFSIZ];
    int    *iptr;
    int     index,i;
    char   *str;

    iptr = (int *) vp->varptr;
    (void) sprintf(string, "%s[0..%d] index:(name or number) ", 
    						vp->name, vp->max_x - 1);

    while ((str = get_command(string)) != NULL) {
	if (sscanf(str,"%d",&index) == 0) {
	  for (index = vp->max_x,i = vp->max_y; i < nunames; i++) {
	    if (startsame(str, uname[i])) {
	         index = i - vp->max_y;
	         break;
	    }
 	  }
	}
	if (index >= vp->max_x) {
	    if((index = get_pattern_number(str)) < 0) {
	    	    if (ind_error(vp->name) == BREAK) return(BREAK);
		    goto again;
	    }
	}
	(void) sprintf(tempstring, "%s[%d] = %d, new value: ", 
					vp->name, index, *(iptr + index));
	if ((str = get_command(tempstring)) != NULL) {
	    if (!sscanf(str,"%d",(iptr + index))) {
	    	return(var_error(vp->name,index,-1));
	    }
	}
	return(CONTINUE);
again:
	(void) sprintf(string, "%s[0..%d] index:(name or number) ", 
					vp->name, vp->max_x - 1);
    }
}

change_fvector_var(vp)
struct Variable *vp;
{
    char    string[BUFSIZ];
    char    tempstring[BUFSIZ];
    float  *fptr;
    int     index,i;
    char   *str;

    fptr = (float *) vp->varptr;
    (void) sprintf(string, "%s[0..%d] index:(name or number) ", 
    						vp->name, vp->max_x - 1);

    while ((str = get_command(string)) != NULL) {
	if (sscanf(str,"%d",&index) == 0) {
	  for (index = vp->max_x,i = vp->max_y; i < nunames; i++) {
	    if (startsame(str, uname[i])) {
	         index = i - vp->max_y;
	         break;
	    }
 	  }
	}
	if (index >= vp->max_x) {
	    if((index = get_pattern_number(str)) < 0) {
	    	    if (ind_error(vp->name) == BREAK) return(BREAK);
		    goto again;
	    }
	}
	(void) sprintf(tempstring, "%s[%d] = %.3f, new value: ", 
					vp->name, index, *(fptr + index));
	if ((str = get_command(tempstring)) != NULL) {
	    if (!sscanf(str,"%f",(fptr + index))) {
	    	return(var_error(vp->name,index,-1));
	    }
	}
	return(CONTINUE);
again:
	(void) sprintf(string, "%s[0..%d] index:(name or number) ", 
						vp->name, vp->max_x - 1);
    }
}

change_svector_var(vp)
struct Variable *vp;
{
    char    string[BUFSIZ];
    int     index,i;
    char   *str;
    char  **sptr;
    char   *emalloc ();

    sptr = (char **) vp->varptr;
    (void) sprintf(string, "%s[0..%d] index:(name or number) ", 
    					vp->name, vp->max_x - 1);

    while ((str = get_command(string)) != NULL) {
	 if (sscanf(str,"%d",&index) == 0) {
	   if((index = get_pattern_number(str)) < 0) {
	      for (index = vp->max_x,i = vp->max_y; i < nunames; i++) {
	        if (startsame(str, uname[i])) {
	             index = i - vp->max_y;
	             break;
	        }
 	      }
	      if (index >= vp->max_x) {
	      	    if (ind_error(vp->name) == BREAK) return(BREAK);
		    goto again;
	      }
	   }
	}
	if (sptr[index] != NULL) {
	    (void) sprintf(string, "%s[%d] = %s, new value: ", 
	    				vp->name, index, sptr[index]);
	}
	else {
	    (void) sprintf(string, "%s[%d] = empty, new value: ", 
	    						vp->name, index);
	}
	if ((str = get_command(string)) != NULL) {
	    if (sptr[index] != NULL) free(sptr[index]);
	    sptr[index] = 
	      (char *) emalloc((unsigned)(sizeof(char) * (strlen(str) + 1)));
	    (void) strcpy(sptr[index], str);
	}
        return(CONTINUE);
again:
	(void) sprintf(string, "%s[0..%d] index:(name or number) ", 
						vp->name, vp->max_x - 1);
    }
}

change_pfvector_var(vp,iswv)
struct Variable *vp;
int iswv;  /* is it a weight vector */
{
    char    string[BUFSIZ];
    float  **pfptr;
    int     i,index1,index2,cmin,cmax,tindex2;
    char   *str;
    char    *name;

    pfptr = (float **) vp->varptr;
    name = (char *) vp->name;

    (void) sprintf(string, "%s[0..%d][0..%d] row index:(name or number)  ",
	    vp->name, vp->max_x - 1, vp->max_y - 1);

    while ((str = get_command(string)) != NULL) {
	if (sscanf(str,"%d",&index1) == 0) {
	  for (index1 = vp->max_x,i = 0; i < nunames; i++) {
	    if (startsame(str, uname[i])) {
	         index1 = i;
	         break;
	    }
 	  }
	  if (index1 >= vp->max_x) {
	   for (index1 = vp->max_x,i = 0; i < npatterns; i++) {
	    if (startsame(str, pname[i])) {
	         index1 = i;
	         break;
	    }
 	   }
	  }
	}
	if (index1 >= vp->max_x) {
	    if (ind_error(vp->name) == BREAK) return(BREAK);
	    goto again;
	}
	if (iswv) {
	    cmin = first_weight_to[index1];
	    cmax = cmin + num_weights_to[index1] -1;
	}
	else {
	    cmin = 0;
	    cmax = vp->max_y - 1;
	}
	(void) sprintf(string, "%s[%d][%d..%d] column index:(name or number)  ", 
		       vp->name, index1, cmin,cmax);
	while ((str = get_command(string)) != NULL) {
	    if (sscanf(str,"%d",&index2) == 0) {
	      for (index2 = vp->max_y,i = 0; i < nunames; i++) {
	        if (startsame(str, uname[i])) {
	         index2 = i;
		 if (strcmp(name,"tpattern") == 0) 
		    index2 -= nunits - noutputs;
	         break;
	        }
	      }
	    }
	    if (iswv) tindex2 = index2 - first_weight_to[index1];
	    else tindex2 = index2;
	    if (index2 > cmax || index2 < cmin) {
	        if (ind_error(vp->name) == BREAK) return(BREAK);
		goto i2_again;
	    }
	    (void) sprintf(string, "%s[%d][%d] = %.3f, new value: ", 
			   vp->name, index1, index2, 
			   pfptr[index1][tindex2]);
	    if ((str = get_command(string)) != NULL) {
	      if(!sscanf(str,"%f",&pfptr[index1][tindex2])) {
	      	return(var_error(vp->name,index1,index2));
	      }
	    }
	    return(CONTINUE);
i2_again:
	    (void) sprintf(string,"%s[%d][%d..%d] column index:(name or number)  ",
			   vp->name, index1,cmin,cmax );
	}
again:
	(void)sprintf(string,"%s[0..%d][0..%d] row index:(name or number)  ",
		vp->name, vp->max_x - 1, vp->max_y - 1);
    }
}

get_unames() {
    int i;

    if (nunits == 0) {
	return(put_error("Must define nunits before unames!"));
    }

    if(uname == NULL) {
        uname = (char **) emalloc((unsigned)(sizeof(char *) * nunits));
	for (i = 0; i < nunits; i++)
		uname[i] = NULL;
        install_var("uname",Vstring,(int *) uname, nunames, 0, 
								SETCONFMENU);
    }

    /* we do one extra to read the "end" at the end of the list */
    for (nunames = 0; nunames < nunits+1; nunames++) {
	if (getn(&uname[nunames]) == FALSE || nunames == nunits) {
	    break;
	}
    }
    change_variable_length("uname",nunames,0);
    return(CONTINUE);
}

getn(nm) char **nm; {
    char   *str;

    str = get_command("next name (terminate with end): ");
    if (str == NULL)
	return(FALSE);
    if (strcmp(str, "end") == 0) {
	return(FALSE);
    }
    *nm = strsave(str);
    return(TRUE);
}

var_error(vname,i1,i2) char *vname; int i1,i2; {
	if (i1 < 0) {
		sprintf(err_string,"illegal value given for %s.",vname);
	}
	else if (i2 < 0) {
		sprintf(err_string,"illegal value given for %s[%d].",
							vname,i1);
	}
	else {
		sprintf(err_string,"illegal value given for %s[%d][%d].",
							vname,i1,i2);
	}
	return(put_error(err_string));
}

ind_error(vname) char *vname; {
	sprintf(err_string,"illegal index encountered for %s.",vname);
	return (put_error(err_string));
}
 
