/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* file: cl.c

	System file for competitive learning.
	
	First version implemented by Elliot Jaffe
	
	Date of last revision:  8-12-87/JLM
*/

#include "general.h"
#include "variable.h"
#include "patterns.h"
#include "command.h"
#include "cl.h"

char   *Prompt = "cl: ";
char   *Default_step_string = "epoch";
boolean System_Defined = FALSE;

boolean lflag = 1;
int     nepochs = 20;
int     epochno = 0;
float   lrate = 0.2;

int	nunits = 0;
int     noutputs = 0;		/* number of units in competitive pool */
int     ninputs = 0;		/* number of units in input layer */
float  **weight;		/* pointers to vectors of weights */
int    *activation = NULL;	/* activations for all units */
float  *netinput = NULL;	/* sum of inputs for pool units */

int	tallflag = 0;

int     patno = 0;
int	winner = 0;

define_system() {
    int     i,j;

    if (noutputs <= 0) {
	put_error("cannot initialize weights without noutputs");
	return(FALSE);
    }

    if (ninputs <= 0) {
	put_error("cannot initialize weights without ninputs");
	return(FALSE);
    }

    nunits = ninputs + noutputs;

    if (activation != NULL) {
	free((char *) activation);
    }

    if (netinput != NULL) {
	free((char *) netinput);
    }

    activation = (int *) emalloc((unsigned)(nunits * sizeof(int)));
    install_var("activation", Vint,(int *) activation, nunits,0,SETSVMENU);
    netinput = (float *) emalloc((unsigned)(nunits * sizeof(float)));
    install_var("netinput", Vfloat,(int *) netinput, nunits,0,SETSVMENU);

    weight = ((float **) emalloc((unsigned)(nunits*sizeof(float *))));
    
    for (i = ninputs; i < nunits; i++) {
	weight[i] = ( (float *) emalloc ( (unsigned) ninputs * sizeof (float) ));
	for (j = 0; j < ninputs; j++) {
	    weight[i][j] = 0.0;
	}
    }
    install_var("weight", PVweight,(int *) weight, nunits, nunits,SETWTMENU);
       
    first_weight_to = (int *) emalloc((unsigned)(sizeof(int) * nunits));
    num_weights_to = (int *) emalloc((unsigned)(sizeof(int) * nunits));

    for (i = 0; i < ninputs; i++) {
	first_weight_to[i] = 0;
	num_weights_to[i] = 0;
    }
    for ( ; i < nunits; i++) {
	first_weight_to[i] = 0;
	num_weights_to[i] = ninputs;
    }
	
    for (i = 0; i < nunits; i++) {
	activation[i] = 0;
    }

    for (i = ninputs; i < nunits; i++) {
	netinput[i] = 0.0;
        for (j = 0; j < ninputs; j++) {
	    netinput[i] += weight[i][j] = rnd();
	}
	for (j = 0; j < ninputs; j++) {
	    weight[i][j] *= 1.0/netinput[i];
	}
	activation[i] = 0;
	netinput[i] = 0.0;
    }

    System_Defined = TRUE;

    return(TRUE);
}

get_weights() {
    register int    i,
                    j;
    char   *str;
    FILE * iop;

    if(! System_Defined)
      if(! define_system())
       return(BREAK);

    str = get_command("fname: ");
    if (str == NULL) return(CONTINUE);
    if ((iop = fopen_read_compat(str)) == NULL) {
	return(put_error("Cannot open file"));
    }
    for (i = ninputs; i < nunits; i++) {
	for (j = 0; j < ninputs; j++) {
	    (void) fscanf(iop, "%f", &weight[i][j]);
	}
	(void) fscanf(iop, "\n");
    }

    epochno = 0;

    for (i = ninputs; i < nunits; i++) {
      netinput[i]=activation[i] = 0.0;
    }
    for (i = 0; i < ninputs; i++) {
      activation[i] = 0.0;
    }
    update_display();
    fclose(iop);
    return(CONTINUE);
}


save_weights() {
    register int    i,
                    j;
    float  *fp;
    FILE * iop;
    char   *str;
    char tstr[40];
    char fname[BUFSIZ];
    char *star_ptr;

    if(! System_Defined)
      if(! define_system())
       return(BREAK);

nameagain:
    str = get_command("file name: ");
    if (str == NULL) return(CONTINUE);
    if ( (star_ptr = index(str,'*')) != NULL) {
    	strcpy(tstr,star_ptr+1);
    	sprintf(star_ptr,"%d",epochno);
	strcat(str,tstr);
    }
    strcpy(fname,str);
    if ((iop = fopen(fname, "r")) != NULL) {
    	fclose(iop);
        get_command("file exists -- clobber? ");
	if (str == NULL || str[0] != 'y') {
	   goto nameagain;
	}
    }
    if ((iop = fopen(fname, "w")) == NULL) {
	return(put_error("cannot open file for weights"));
    }
    for (i = ninputs; i < nunits; i++) {
	for (j = 0; j < ninputs; j++) {
	    fprintf(iop, "%6.3f", weight[i][j]);
	}
	fprintf(iop, "\n");
    }
    (void) fclose(iop);
    return(CONTINUE);
}

compute_output() {
    int     i, j;

    for (i = ninputs; i < nunits; i++) {
	netinput[i] = 0.0;
	activation[i] = 0;
    }

    for (i = 0; i < ninputs; i++) {
	if (activation[i]) {
	    for (j = ninputs; j < nunits; j++) {
		netinput[j] += weight[j][i];
	    }
	}
    }

    for (winner = ninputs, i = ninputs; i < nunits; i++) {
	if (netinput[winner] < netinput[i]) {
	    winner = i;
	}
    }

    activation[winner] = 1;
}

change_weights()
{
    int     i;
    float   nactive = 0;

    for (i = 0; i < ninputs; i++) {
	if (activation[i])
	    nactive += 1;
    }

    if(nactive == 0) return;

    for (i = 0; i < ninputs; i++) {
	weight[winner][i] += 
	  lrate * ((activation[i] / nactive) 
		   	- weight[winner][i]);
    }
}

setinput() {
    register int    i;
    register float  *pp;

    for (i = 0, pp = ipattern[patno]; i < ninputs; i++, pp++) {
	    activation[i] = *pp;
    }
    strcpy(cpname,pname[patno]);
}


setup_pattern() {
    setinput();
}

trial() {
    setup_pattern();
    compute_output();
}


ptrain() {
  return(train('p'));
}

strain() {
  return(train('s'));
}

train(c) char c; {
    int     t,i,old,npat;
    char    *str;

    if (!System_Defined)
	if (!define_system())
	    return(BREAK);

    for (t = 0; t < nepochs; t++) {
	if (!tallflag) epochno++;
	for (i = 0; i < npatterns; i++)
	    used[i] = i;
	if (c == 'p') {
	  for (i = 0; i < npatterns; i++) {
	    npat = rnd() * (npatterns - i) + i;
	    old = used[i];
	    used[i] = used[npat];
	    used[npat] = old;
	  }
	}
	for (i = 0; i < npatterns; i++) {
	    if (Interrupt) {
		Interrupt_flag = 0;
		update_display();
	        if (contin_test() == BREAK) return(BREAK);
	    }
	    patno = used[i];
	    trial();
	    if (lflag) change_weights();
	    if (step_size == PATTERN) {
	      update_display();
	      if (single_flag) {
	         if (contin_test() == BREAK) return(BREAK);
	      }
	    }
	}
	if (step_size == EPOCH) {
	 update_display();
	 if (single_flag) {
	         if (contin_test() == BREAK) return(BREAK);
	 }
        }
    }
    if (step_size == NEPOCHS) {
    	update_display();
    }
    return(CONTINUE);
}

tall() {
  int save_lflag;
  int save_single_flag;
  int save_nepochs;
  int save_step_size;
  
  save_lflag = lflag;  lflag = 0;
  save_single_flag = single_flag; 
  if (in_stream == stdin) single_flag = 1;
  save_nepochs = nepochs;  nepochs = 1;
  save_step_size = step_size;
  if (step_size > PATTERN) step_size = PATTERN;
  tallflag = 1;
  train('s');
  tallflag = 0;
  lflag = save_lflag;
  nepochs = save_nepochs;
  single_flag = save_single_flag;
  step_size = save_step_size;
  return(CONTINUE);
}
  
test_pattern() {
    char   *str;

    if (!System_Defined)
	if (!define_system())
	    return(BREAK);

    str = get_command("Test which pattern? ");
    if(str == NULL) return(CONTINUE);
    if ((patno = get_pattern_number(str)) < 0 ) {
	return(put_error("Invalid pattern specification."));
    }
    trial();
    update_display();
    return(CONTINUE);
}

newstart() {
	random_seed = rand();
	reset_weights();
}

reset_weights() {
    register int    i,j;
    
    epochno = 0;
    cpname[0] = '\0';

    if (!System_Defined)
	if (!define_system())
	    return(BREAK);


    srand(random_seed);
    
    for (j = ninputs; j < nunits; j++) {
	netinput[j] = 0;
	for (i = 0; i < ninputs; i++) {
		netinput[j] += weight[j][i] = rnd();
	}
    }

    for (j = ninputs; j < nunits; j++) {
	for (i = 0; i < ninputs; i++) {
		weight[j][i] *= 1.0/netinput[j];
	}
    }

    for (i = ninputs; i < nunits; i++) {
      netinput[i]=activation[i] = 0.0;
    }
    for (i = 0; i < ninputs; i++) {
      activation[i] = 0.0;
    }
    update_display();
    return(CONTINUE);
}

init_system() {
    int     strain(), ptrain(), tall(), test_pattern(),reset_weights();
    int	    get_unames();

    install_command("strain", strain, BASEMENU,(int *) NULL);
    install_command("ptrain", ptrain, BASEMENU,(int *) NULL);
    install_command("tall", tall, BASEMENU,(int *) NULL);
    install_command("test", test_pattern, BASEMENU,(int *) NULL);
    install_command("newstart",newstart,BASEMENU,(int *)NULL);
    install_command("reset",reset_weights,BASEMENU,(int *)NULL);
    install_command("weights", get_weights, GETMENU,(int *) NULL);
    install_command("weights", save_weights, SAVEMENU,(int *) NULL);
    install_command("patterns", get_patterns, GETMENU,(int *) NULL);
    install_command("unames", get_unames, GETMENU,(int *) NULL);
    install_var("noutputs", Int,(int *) & noutputs, 0, 0, SETCONFMENU);
    install_var("ninputs", Int,(int *) & ninputs, 0, 0, SETCONFMENU);
    install_var("nunits", Int,(int *) & nunits, 0, 0, SETCONFMENU);
    install_var("lrate", Float,(int *) & lrate, 0, 0, SETPARAMMENU);
    install_var("lflag", Int,(int *) & lflag, 0, 0, SETPCMENU);
    install_var("nepochs", Int,(int *) & nepochs, 0, 0, SETPCMENU);
    install_var("epochno", Int,(int *) & epochno, 0, 0, SETSVMENU);
    install_var("patno", Int,(int *) & patno, 0, 0, SETSVMENU);
    
    init_patterns();
}
