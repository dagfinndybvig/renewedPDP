/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* file: patterns.c

	This file contains functions for reading patterns.
	
	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

/*LINTLIBRARY*/

#include "general.h"
#include "command.h"
#include "variable.h"
#include "patterns.h"

int     npatterns;
int	maxpatterns = MAXPATTERNS;
float  **ipattern;
float  **tpattern;
char   **pname;
int    *used;
char   cpname[BUFSIZ];

extern int ninputs; 
extern int noutputs;

init_pattern_pairs() {
    init_pats(PAIRS);
}

init_patterns() {
    init_pats(!PAIRS);
}

init_pats(pairs) int pairs; {

    install_var("npatterns", Int,(int *) & npatterns, 0, 0,SETENVMENU);
    install_var("maxpatterns",Int,(int *) &maxpatterns,0,0,SETENVMENU);
    install_var("ipattern", PVfloat,(int *) ipattern, 0, 0,SETENVMENU);
    if (pairs) 
      install_var("tpattern", PVfloat,(int *) tpattern,0,0,SETENVMENU);
    install_var("pname",Vstring,(int *) pname, 0, 0, SETENVMENU);
    install_var("cpname", String,(int *) cpname, 0, 0, SETSVMENU);
    
}

reset_patterns(pairs) int pairs; {
    int i;
    struct Variable *vp;
    
    if (pname) {
      for (i = 0; i < npatterns; i++) {
	if(pname[i] != NULL)
	  free(pname[i]);
      }
      free(pname);
    }
    pname = ( (char **) emalloc( (unsigned)(sizeof (char *) * maxpatterns)));
    vp = lookup_var("pname");
    vp->varptr = (int *) pname;
    if (ipattern) {
      for(i = 0; i < npatterns; i++) {
	if(ipattern[i] != NULL)
	  free(ipattern[i]);
      }
      free(ipattern);
    }
    ipattern = 
    	((float **) emalloc( (unsigned) (sizeof (float *) * maxpatterns)));
    vp = lookup_var("ipattern");
    vp->varptr = (int *) ipattern;
    if (pairs) {
      if (tpattern) {
        for (i = 0; i < npatterns; i++) {
         if(tpattern[i] != NULL)
	  free(tpattern[i]);
	}
	free(tpattern);
      }
      tpattern = 
	  ((float **) emalloc( (unsigned) (sizeof (float *) * maxpatterns)));
      vp = lookup_var("tpattern");
      vp->varptr = (int *) tpattern;
    }
    if (used) free(used);
	used = ( (int *) emalloc( (unsigned) (sizeof (int) * maxpatterns)));
}

enlarge_patterns(pairs) int pairs; {
    struct Variable *vp;
    int oldmaxpatterns;
    
    oldmaxpatterns = maxpatterns;
    
    maxpatterns += 100;
    pname = ( (char **) 
      erealloc( (char *) pname,
          (unsigned)(oldmaxpatterns * sizeof(char *)),
	  (unsigned)(maxpatterns * sizeof(char *))));
    vp = lookup_var("pname");
    vp->varptr = (int *) pname;
    ipattern = ( (float **) 
      erealloc( (char *)ipattern,
          (unsigned)(oldmaxpatterns * sizeof(float *)),
	  (unsigned)(maxpatterns * sizeof(float *))));
    vp = lookup_var("ipattern");
    vp->varptr = (int *) ipattern;
    if (pairs) {
	tpattern = ( (float **) 
         erealloc( (char *)tpattern,
	     (unsigned)(oldmaxpatterns * sizeof(float *)),
	     (unsigned)(maxpatterns * sizeof(float *))));
	vp = lookup_var("tpattern");
	vp->varptr = (int *) tpattern;
    }
    used = ( (int *) erealloc( (char *)used,
    		(unsigned)(oldmaxpatterns * sizeof(int)),
		(unsigned)(maxpatterns * sizeof(int))));
}

get_patterns() {
    return(get_pats(!PAIRS));
}

get_pattern_pairs() {
    return(get_pats(PAIRS));
}

get_pats(pairs) int pairs; {
    char   *sp;
    int     i,j,rval;
    FILE   *iop;
    char    temp[LINE_SIZE];

    if (!System_Defined)
      if (!define_system())
	return(put_error("Define network before getting patterns"));

    sp = get_command("filename for patterns: ");
    if( sp == NULL) return(CONTINUE);
    if ((iop = fopen(sp, "r")) == NULL) {
	return(put_error("Can't open file for patterns."));
    }
    reset_patterns(pairs);
    for (i = 0; 1; i++) {
	if (fscanf(iop,"%s",temp) == EOF) {
	    break;
	}
	if (i == maxpatterns) {
	    enlarge_patterns(pairs);
	}
	pname[i] = ((char *) emalloc((unsigned)(strlen(temp) + 1)));
	(void) strcpy(pname[i], temp);
	ipattern[i] = ((float *) emalloc((unsigned) (ninputs*sizeof(float))));
	for (j = 0; j < ninputs; j++) {
	  if (fscanf(iop,"%s",temp) == EOF) {
	    rval = put_error("Pattern file structure does not match specs!");
	    goto pattern_end;
	  }
	  if (strcmp(temp,"+") == 0) ipattern[i][j] = (float) 1.0;
	  else if (strcmp(temp,"-") == 0) ipattern[i][j] = (float) -1.0;
	  else if (strcmp(temp,".") == 0) ipattern[i][j] = (float) 0.0;
	  else if (sscanf(temp,"%f",&ipattern[i][j]) != 1) {
	    rval = put_error("Pattern file structure does not match specs!");
	    goto pattern_end;
	  }
	}
	if (pairs) {
	 tpattern[i] = ((float *)emalloc((unsigned)(noutputs*sizeof(float))));
	 for (j = 0; j < noutputs; j++) {
	  if (fscanf(iop,"%s",temp) == EOF) {
	    rval = put_error("Pattern file structure does not match specs!");
	    goto pattern_end;
	  }
	  if (strcmp(temp,"+") == 0) tpattern[i][j] = (float) 1.0;
	  else if (strcmp(temp,".") == 0) tpattern[i][j] = (float) 0.0;
	  else if (strcmp(temp,"-") == 0) tpattern[i][j] = (float) -1.0;
	  else if (sscanf(temp,"%f",&tpattern[i][j]) != 1) {
	    rval = put_error("Pattern file structure does not match specs!");
	    goto pattern_end;
	  }
	 }
	}
    }
    rval = CONTINUE;
pattern_end:
    npatterns = i;
    change_variable_length("ipattern",npatterns,ninputs);
    if (pairs) 
        change_variable_length("tpattern",npatterns,noutputs);
    change_variable_length("pname",npatterns,0);
    clear_display();
    update_display();
    (void) fclose(iop);
    return(rval);
}

get_pattern_number(str)  char *str; {

    int index;

    if(sscanf(str,"%d",&index) == 1) 
	if(index < 0 || index >= npatterns)
	    return(-1);
	else
	    return(index);
    for(index = 0; index < npatterns; index++)
	if(startsame(str,pname[index]))
	   break;

    if(index < npatterns)
	return(index);
    else
	return(-1);
}
