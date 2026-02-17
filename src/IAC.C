/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* file: iac.c

	Do the actual work for interactive activation and competition
	networks.
	
	First implemented by JLM
	
	Date of last revision:  8-12-87/JLM.

*/

#include "general.h"
#include "iac.h"
#include "variable.h"
#include "command.h"
#include "weights.h"
#include "patterns.h"

char   *Prompt = "iac: ";
char   *Default_step_string = "cycle";
boolean System_Defined = FALSE;


float	    maxactiv = 1;
float	    minactiv = -.2;

float  *activation;
float  *netinput;
float  *extinput;
float  *inhibition;
float  *excitation;

float   estr = .1;
float   alpha = .1;
float	gamma = .1;
float   decay = .1;
float	rest = -.1;
float	dtr = -.01;  /* decay times rest, for efficiency */
float	omd = .9;    /* 1 - decay, for efficiency */
int	epochno = 0; /* not used in iac */
int	patno = 0;
int     ncycles = 10;
int     cycleno = 0;
int	gb = 0; /* for grossberg mode */

/* first get the number of units, then malloc all the necessary
   data structures.
*/
define_system() {
    int     i;

    activation = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    install_var("activation",Vfloat,(int*)activation,nunits,0,SETSVMENU);
    for (i = 0; i < nunits; i++)
	activation[i] = 0.0;

    netinput = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    install_var("netinput",Vfloat,(int*)netinput,nunits,0,SETSVMENU);
    for (i = 0; i < nunits; i++)
	netinput[i] = 0.0;

    excitation = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    install_var("excitation",Vfloat,(int*)excitation,nunits,0,SETSVMENU);

    inhibition = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    install_var("inhibition",Vfloat,(int*)inhibition,nunits,0,SETSVMENU);

    extinput = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    install_var("extinput",Vfloat,(int*)extinput,nunits,0,SETSVMENU);
    for (i = 0; i < nunits; i++)
	excitation[i] = inhibition[i] = extinput[i] = 0.0;

    System_Defined = TRUE;
    zarrays();

    return(TRUE);
}

getinput() {
    register int    j;
    char   *str;

    if (!System_Defined)
	if (!define_system())
	    return(BREAK);

    str = get_command("pattern: ");
    if (str == NULL) return(CONTINUE);
    for (j = 0; j < nunits; j++) {
	if (str[j] == '1' || str[j] == '+')
	    extinput[j] = 1.;
	else
	    if (str[j] == '-')
		extinput[j] = -1.;
	else
	    extinput[j] = 0.;
    }
    return(CONTINUE);
}

zarrays() {
    register int    i;

    cycleno = 0;

    if (!System_Defined)
	if (!define_system())
	    return(BREAK);

    for (i = 0; i < nunits; i++) {
	excitation[i] = inhibition[i] = netinput[i] = 0;
	activation[i] = rest;
    }
    return(CONTINUE);
}

cycle() {
    int     iter;
    char *str;

    if (!System_Defined)
	if (!define_system())
	    return(BREAK);

    for (iter = 0; iter < ncycles; iter++) {
	cycleno++;
        getnet();
	update();
	if (step_size == CYCLE) {
	  update_display();
	  if (single_flag) {
	    if (contin_test() == BREAK) return(BREAK);
	  }
	}
	if (Interrupt) {
	    update_display();
	    Interrupt_flag = 0;
	    if (contin_test() == BREAK) return(BREAK);
	}
    }
    if (step_size == NCYCLES) {
    	update_display();
    }
    return(CONTINUE);
}

getnet() {
    register int    i, j;
    register int wi, *fwp, *nwp; 
    float a,w;
    
    
    for (i = 0; i < nunits; i++) {
	excitation[i] = inhibition[i] = 0;
    }
    for (j = 0; j < nunits; j++) {
	if ( (a = activation[j]) > 0.0) {
	  for (i = 0,fwp = first_weight_to,nwp = num_weights_to; 
	       i < nunits; i++) {
	   wi = j - *fwp++;
	   if ( (wi >= *nwp++) || (wi < 0) ) continue;
	   if ( (w = weight[i][wi]) > 0.0 ) {
	            excitation[i] += a * w;
	   }
	   else if (w <0.0) {
	            inhibition[i] += a * w;
	   }
	  }
	}
    }
    for (i = 0; i < nunits; i++) {
       	excitation[i] *= alpha;
	inhibition[i] *= gamma;
	if (gb) {
	    if (extinput[i] > 0) excitation[i] += estr*extinput[i];
	    else if (extinput[i] < 0) inhibition[i] += estr*extinput[i];
	}
	else {
            netinput[i] =  excitation[i] + inhibition[i] + 
						estr * extinput[i];
	}
    }
}

update () {
    register float *act, *ex, *in, *net, *end;
    
    if (gb) {
      for (act=activation, ex=excitation, in=inhibition,end = act+nunits;
           act < end; act++) {
	*act = (*ex++) * (maxactiv - (*act)) + (*in++)*((*act)-minactiv) 
		+ omd*(*act) + dtr;
	/* omd * a + dtr equals a - decay*(a - rest) */
	if (*act > maxactiv) *act = maxactiv;
	else if (*act < minactiv) *act = minactiv;
      }
    }
    else {
      for (act = activation, net = netinput, end = act + nunits;
           act < end; act++, net++) {
	if (*net > 0) {
		*act = (*net) * (maxactiv - (*act)) + omd*(*act) + dtr;
		if (*act > maxactiv) *act = maxactiv;
	}
	else {
		*act = (*net) * ((*act) - minactiv) + omd*(*act) + dtr;
		if (*act < minactiv) *act = minactiv;
	}
      }
    }
}

input() {
    int     i;
    char   *str,tstr[100];

    if (!System_Defined)
	if (!define_system())
	    return(BREAK);
    if (!nunames) {
    	return(put_error("Must provide unit names. "));
    }
again:
    str = get_command("Do you want to reset all inputs?: (y or n)");
    if (str == NULL) goto again;
    if (str[0] == 'y') {
	    for (i = 0; i < nunits; i++)
		extinput[i] = 0;
    }
    else if (str[0] != 'n') {
    	put_error ("Must enter y or n!");
	goto again;
    }

gcname: 
    str = get_command("give unit name or number: ");
    if (str == NULL || (strcmp(str,"end") == 0) ) {
	update_display();
	return(CONTINUE);
    }
    if (sscanf(str,"%d",&i) == 0) {
        for (i = 0; i < nunames; i++) {
	    if (startsame(str, uname[i])) break;
        }
    }
    if (i >= nunames) {
	if (put_error("unrecognized name -- try again.") == BREAK) {
		return(BREAK);
	}
	goto gcname;
    }
gcval: 
    sprintf(tstr,"enter input strength of %s:  ",uname[i]);
    str = get_command(tstr);
    if (str == NULL) {
    	sprintf(err_string,"no strength specified of %s.",uname[i]);
    	if (put_error(err_string) == BREAK) {
		return(BREAK);
	}
	goto gcname;
    }
    if (sscanf(str, "%f", &extinput[i]) != 1) {
	if (put_error("unrecognized value.") == BREAK) {
		return(BREAK);
	}
	goto gcval;
    }
    goto gcname;
}

setinput() {
    register int    i;
    register float  *pp;

    for (i = 0, pp = ipattern[patno]; i < nunits; i++, pp++) {
            extinput[i] = *pp;
    }
    strcpy(cpname,pname[patno]);
}

test_pattern() {
    char   *str;
 
    if (!System_Defined)
        if (!define_system())
            return(BREAK);
 
    if(ipattern[0] == NULL) {
       return(put_error("No file of test patterns has been read in."));
    }
again:
    str = get_command("Test which pattern? (name or number): ");
    if(str == NULL) return(CONTINUE);
    if ((patno = get_pattern_number(str)) < 0) {
    	return(put_error("Invalid pattern specification."));
    }
    setinput();
    zarrays();
    cycle();
    return(CONTINUE);

}

reset_system() {
    clear_display();
    zarrays();
/*    reset_display(); moving vector reset -- out of service */
    update_display();
    return(CONTINUE);
}

constrain_weights() {
}


init_system() {
    int get_unames(),test_pattern(),read_weights(),write_weights(),
    change_decay(), change_rest();

    epsilon_menu = NOMENU;

    install_command("cycle",cycle, BASEMENU, (int *)NULL);
    install_command("input", input, BASEMENU,(int *) NULL);
    install_command("test", test_pattern, BASEMENU,(int *) NULL);
    install_command("network", define_network, GETMENU,(int *) NULL);
    install_command("weights", read_weights, GETMENU,(int *) NULL);
    install_command("patterns", get_patterns, GETMENU,(int *) NULL);
    install_command("unames", get_unames, GETMENU,(int *) NULL);
    install_command("reset", reset_system, BASEMENU,(int *) NULL);
    install_command("weights", write_weights, SAVEMENU,(int *) NULL);
    install_var("gb",Int,(int *) & gb, 0, 0, SETMODEMENU);
    install_var("patno", Int,(int *) & patno, 0, 0, SETSVMENU);
    init_patterns();
    install_var("cycleno", Int,(int *) & cycleno, 0, 0, SETSVMENU);
    install_var("ncycles", Int,(int *) & ncycles, 0, 0, SETPCMENU);
    install_var("nunits", Int,(int *) & nunits, 0, 0, SETCONFMENU);
    install_var("ninputs", Int,(int *) & ninputs, 0, 0, SETCONFMENU);
    install_var("estr", Float,(int *) & estr, 0, 0,
		       					SETPARAMMENU);
    install_var("alpha", Float,(int *) & alpha, 0, 0, SETPARAMMENU);
    install_var("gamma", Float,(int *) & gamma, 0, 0, SETPARAMMENU);
    install_var("decay", Float,(int *) & decay, 0, 0, NOMENU);
    install_command("decay",change_decay, SETPARAMMENU, (int *) NULL);
    install_var("max",Float, (int *) & maxactiv, 0, 0, 
		       					SETPARAMMENU);
    install_var("min",Float, (int *) & minactiv, 0, 0, 
							SETPARAMMENU);
    install_var("rest", Float,(int *) & rest, 0, 0, NOMENU);
    install_command("rest",change_rest, SETPARAMMENU, (int *) NULL);
    
}

change_rest() {
    struct Variable *varp;

    if ((varp = lookup_var("rest")) != NULL) {
	change_variable("rest",(int *) varp);
    }
    else {
	return(put_error("rest is not defined"));
    }
    dtr = decay * rest;
    return(CONTINUE);
}

change_decay() {
    struct Variable *varp;

    if ((varp = lookup_var("decay")) != NULL) {
	change_variable("decay",(int *) varp);
    }
    else {
	return(put_error("decay is not defined"));
    }
    dtr = decay * rest;
    omd = 1 - decay;
    return(CONTINUE);
}

