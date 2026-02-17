/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* file: cs.c

	Do the actual work for the cs program.
	
	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

#include "general.h"
#include "cs.h"
#include "variable.h"
#include "command.h"
#include "patterns.h"
#include "weights.h"

#define	 MAXTIMES	20
#define  FMIN (1.0e-37)
#define  fcheck(x) (fabs(x) > FMIN ? (float) x : (float) 0.0)

char   *Prompt = "cs: ";
char   *Default_step_string = "cycle";
boolean System_Defined = FALSE;

boolean     clamp = 0;
boolean	    boltzmann = 0;
boolean	    harmony = 0;

float	    temperature,coolrate;
float	    goodness;

float  *activation;
float  *netinput;
float  *intinput;
float  *extinput;

float   estr = 1.0;
float   istr = 1.0;

float	kappa;

int	epochno = 0; /* not used in cs */
int	patno = 0;
int     ncycles = 10;
int     nupdates = 100;
int 	cycleno = 0;
int     updateno = 0;
int	unitno = 0;
char	cuname[40];

int	ntimes = 0;

struct anneal_schedule {
	int	time;
	float	temp;
}  *anneal_schedule;

int maxtimes = MAXTIMES;

struct anneal_schedule *last_temp;
struct anneal_schedule *next_temp;
struct anneal_schedule *current_temp;

define_system() {
    int     i,j;
    float   *tmp;

    activation = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    (void)install_var("activation",Vfloat,(int *)activation,nunits,0,
								SETSVMENU);
    for (i = 0; i < nunits; i++)
	activation[i] = 0.0;

    netinput = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    (void)install_var("netinput",Vfloat,(int *)netinput,nunits,0,SETSVMENU);
    for (i = 0; i < nunits; i++)
	netinput[i] = 0.0;

    intinput = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    (void)install_var("intinput",Vfloat,(int *)intinput,nunits,0,SETSVMENU);
    for (i = 0; i < nunits; i++)
	intinput[i] = 0.0;

    extinput = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    (void)install_var("extinput",Vfloat,(int *)extinput,nunits,0,SETSVMENU);
    for (i = 0; i < nunits; i++)
	extinput[i] = 0.0;

    anneal_schedule = ((struct anneal_schedule *)
	       emalloc((unsigned)maxtimes*sizeof(struct anneal_schedule)));
		       
    next_temp = anneal_schedule;
    last_temp = anneal_schedule;
    last_temp->time = 0;
    last_temp->temp = 0.0;
    
    constrain_weights();

    System_Defined = TRUE;
    reset_system();
    return(TRUE);
}


double  logistic (i) float   i; {
    double  val;
    double  ret_val;
    double  exp ();

    if( temperature <= 0.0)
	return(i > 0);
    else
        val = i / temperature;

    if (val > 11.5129)
	return(.99999);
    else
	if (val < -11.5129)
	    return(.00001);
    else
	    ret_val = 1.0 / (1.0 + exp(-1.0 * val));
    if (ret_val > FMIN) return(ret_val);
    return(0.0);
}

probability(val) double  val; {
    return((rnd() < val) ? 1 : 0);
}

float   
annealing ( iter) int     iter; {
 /* compute the current temperature given the the last landmark, iteration
           number and the current coolrate We use a simple linear function. */

    double tmp;

    if(iter >= last_temp->time) return(last_temp->temp);
    if(iter >= next_temp->time) {
	tmp = next_temp->temp;
	current_temp = next_temp++;
	coolrate = (current_temp->temp-next_temp->temp)/
		   (float)(next_temp->time - current_temp->time);
    }
    else
	    tmp = current_temp->temp -
		       (coolrate*(float)(iter - current_temp->time));

    return((tmp < FMIN ? 0.0 : tmp));
}

get_schedule() {
    int  cnt;
    char  *str;
    char   string[40];
    struct anneal_schedule *asptr;

    if(!System_Defined)
       if(!define_system())
	  return(BREAK);

    asptr = anneal_schedule;
    next_temp = anneal_schedule;

restart:
    str = get_command("Setting annealing schedule, initial temperature : ");
    if( str == NULL) return(CONTINUE);
    if(sscanf(str,"%f",&(asptr->temp)) == 0) {
       if (put_error("Invalid initial temperature specification.") == BREAK) {
       	return(BREAK);
       }
       goto restart;
    }
    if(asptr->temp < 0) {
       if (put_error("Temperatures must be positive.") == BREAK) {
        return(BREAK);
       }
       goto restart;
    }
    cnt = 1;
    last_temp = asptr++;
    sprintf(string,"time for first milestone:  ");
    while((str = get_command(string)) != NULL) {
       if(strcmp(str,"end") == 0) return(CONTINUE);
       if (cnt >= maxtimes) {
	  maxtimes += 10;
          anneal_schedule = ((struct anneal_schedule *) 
		       erealloc((char *)anneal_schedule,
                     (unsigned)((maxtimes-10)*sizeof(struct anneal_schedule)),
			(unsigned)maxtimes*sizeof(struct anneal_schedule)));
	  next_temp = anneal_schedule;
	  asptr = anneal_schedule + cnt;
	  last_temp = asptr;
       }
       if (sscanf(str,"%d",&asptr->time) == 0) {
          if (put_error("Non_numeric time. ") == BREAK) {
	   return(BREAK);
	  }
	  continue;
       }
       if(asptr->time <= last_temp->time) {
	  if (put_error("Times must increase.") == BREAK) {
	   return(BREAK);
	  }
	  continue;
       }
       sprintf(string,"at time %d the temp should be: ",asptr->time);
       if((str = get_command(string)) == NULL) {
       	  if (put_error("Nothing set at this milestone.") == BREAK) {
	   return(BREAK);
	  }
          goto retry;
       }
       if(sscanf(str,"%f",&(asptr->temp)) == 0) {
       	  if(put_error("Non_numberic temperature.") == BREAK) {
	   return(BREAK);
	  }
          goto retry;
       }
       if(asptr->temp < 0) {
          if(put_error("Temperatures must be positive.") == BREAK) {
	   return(BREAK);
	  }
          goto retry;
       }
       last_temp = asptr++;
       cnt++;
retry:
       sprintf(string,"time for milestone %d: ",cnt);
    }
    return(CONTINUE);
}


cycle() {
    int     iter;
    char	*str;

    if (!System_Defined)
	if (!define_system())
	    return(BREAK);


    for (iter = 0; iter < ncycles; iter++) {
	cycleno++;
	if(boltzmann || harmony)
	    temperature = annealing(cycleno);
	if (rupdate() == BREAK) return(BREAK);
	if(step_size == CYCLE) {
	   get_goodness();
	   cs_update_display();
	   if(single_flag) {
	      if (contin_test() == BREAK) return(BREAK);
	   }
	}
	if (Interrupt) {
	    get_goodness();
	    cs_update_display();
	    Interrupt_flag = 0;
	    if (contin_test() == BREAK) return(BREAK);

	}
    }
    if (step_size == NCYCLES) {
	   get_goodness();
	   cs_update_display();
    }
    return(CONTINUE);
}

get_goodness() {

	int i,j;
	int num, sender, fs, ls; /* fs is first sender, ls is last */
	double dg;

	dg = 0.0;
	
	if(harmony) {
	    for(i=ninputs; i < nunits; i++) {
	       sender = first_weight_to[i];
	       num = num_weights_to[i];
	       for(j = 0; j < num && sender < ninputs; j++,sender++) {
	           dg += 
		       weight[i][j]*activation[i]*activation[sender];
	       }
	       if (activation[i]) dg -= kappa*sigma[i];
	    }
	    goto ret_goodness;
	}
	for(i=0; i < nunits; i++) {
	   fs = first_weight_to[i];
	   ls = num_weights_to[i] + fs -1;
	   for(j = i+1; j < nunits; j++) {
		if ( j < fs ) continue;
		if ( j > ls ) break;
		dg += weight[i][j-fs]*activation[i]*activation[j];
	   }
	   dg += bias[i]*activation[i];
	}
/* >> dont we want to let goodness be affected by istr whether or not
   clamp is 0? Boltz is always clamped, but not schema */
	if(clamp == 0) {
	    dg *= istr;
	    for(i=0; i < nunits; i++) {
		dg += activation[i]*extinput[i]*estr;
	    }
	}
ret_goodness:	
	goodness = dg;
	return;
}

constrain_weights() {
    int    *nconnections;
    int     i,j,num;
    float   value;

    if(!harmony) return;

    nconnections = (int *) emalloc((unsigned)(sizeof(int) * nunits));
    for (i = 0; i < nunits; i++) {
	nconnections[i] = 0;
    }

    for (j = ninputs; j < nunits; j++) {
	num = num_weights_to[j];
	for (i = 0; i < num; i++) {
	    if (weight[j][i])
		nconnections[j]++;
	}
    }

    for (j = ninputs; j < nunits; j++) {
	if (!nconnections[j])
	    continue;
	value = sigma[j] / (float) nconnections[j];
	num = num_weights_to[j];
	for (i = 0; i < num; i++) {
	    if (weight[j][i]) {
		weight[j][i] *= value;
	    }
	}
    }
    free((char *)nconnections);
}

zarrays() {
    register int    i;

    if (!System_Defined)
	if (!define_system())
	    return(BREAK);

    cycleno = 0;

    next_temp = anneal_schedule;
    if(last_temp != next_temp) {
	current_temp = next_temp++;
	coolrate = 
	       (current_temp->temp-next_temp->temp)/(float)next_temp->time;
    }
    temperature = annealing (cycleno);

    goodness = 0;
    updateno = 0;

    for (i = 0; i < nunits; i++) {
	intinput[i] = netinput[i] = activation[i] = 0;
    }
    if (clamp) {
    	init_activations();
    }
    return(CONTINUE);
}

init_activations() {
    register int i;
    for (i = 0; i < nunits; i++) {
	if (extinput[i] == 1.0) {
	    activation[i] = 1.0;
	    continue;
	}
	if (extinput[i] == -1.0) {
	    activation[i] == 0.0;
	    continue;
	}
    }
}

rupdate() {
    register int    j,wi,sender,num,*fwp,*nwp,i,n;
    char *str;
    double dt, inti,neti,acti;

    for (updateno = 0,n = 0; n < nupdates; n++) {
	updateno++;
	unitno = i = randint(0, nunits - 1);
	inti = 0.0;
	if (harmony) {
	     neti = 0.0;
	     if (i < ninputs) {
		if (extinput[i] == 0.0) {
		   for (j = ninputs,fwp = &first_weight_to[ninputs],
		                    nwp = &num_weights_to[ninputs];
				    j < nunits; j++) {
			wi = i - *fwp++;
			if ( (wi >= *nwp++) || (wi < 0) ) continue;
			neti += activation[j]*weight[j][wi];
		   }
		   neti = 2 * neti;
		   if (probability(logistic(neti)))
			activation[i] = 1;
		   else
			activation[i] = -1;
		}
		else {
		   if(extinput[i] < 0.0) activation[i] = -1;
		   if(extinput[i] > 0.0) activation[i] = 1;
		}
	     }
	     else {
	       sender = first_weight_to[i];
	       num = num_weights_to[i];
	       for (j = 0; j < num && sender < ninputs; j++,sender++) {
		      neti += activation[sender]*weight[i][j];
		}
		neti -=  sigma[i]*kappa;
		activation[i] = probability(logistic(neti));
		netinput[i] = neti;
	    }
	}
	else {
	  if (clamp) {
	    if (extinput[i] > 0.0) {
		activation[i] = 1.0;
 		goto end_of_rupdate;
	    }
	    if (extinput[i] < 0.0) {
		activation[i] = 0.0;
		goto end_of_rupdate;
	    }
	  }
	  sender = first_weight_to[i];
	  num = num_weights_to[i];
	  for (j = 0; j < num; j++) {
	    inti += activation[sender++] * weight[i][j];
	  }
	  inti  += bias[i];
	  if (clamp == 0) {
	    neti = istr * inti + estr * extinput[i];
	  }
	  else {
	    neti = istr * inti;
	  }
	  netinput[i] = neti;
	  intinput[i] = inti;
	  if (boltzmann) {
	    if (probability(logistic(neti)))
		activation[i] = 1.0;
	    else
		activation[i] = 0.0;
	  }
	  else {
	    if (neti > 0.0) {
	      if (activation[i] < 1.0) {
		acti = activation[i];
		dt = acti + neti*(1.0 - acti);
		if (dt > 1.0) {
		    activation[i] = (float) 1.0;
                }
		else activation[i] = (float) dt;
              }
            }
	    else {
	      if (activation[i] > (float) 0.0) {
		acti = activation[i];
	        dt = acti + neti * acti;
		if (dt < FMIN) {
		    activation[i] = (float) 0.0;
	        }
		else activation[i] = (float) dt;
	      }
            }
	  }
	}
end_of_rupdate:	
        if(step_size == UPDATE)  {
	   get_goodness();
	   cs_update_display();
	   if (single_flag) {
	      if (contin_test() == BREAK) {
	      	return(BREAK);
	      }
	   }
	}
        if(Interrupt)  {
	   Interrupt_flag = 0;
	   get_goodness();
	   cs_update_display();
	   if (contin_test() == BREAK) {
	      	return(BREAK);
	   }
	}
    }
    return(CONTINUE);
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
    if (str == NULL || strcmp(str,"end") == 0) {
    	if (clamp) init_activations();
	cs_update_display();
	return(CONTINUE);
    }
    if (sscanf(str,"%d",&i) == 0) {
        for (i = 0; i < nunames; i++) {
	    if (startsame(str, uname[i])) break;
        }
    }
    if (i >= nunames) {
	if (put_error("invalid name or number -- try again.") == BREAK) {
		return(BREAK);
	}
	goto gcname;
    }
gcval: 
    sprintf(tstr,"enter input strength of %s:  ",uname[i]);
    str = get_command(tstr);
    if (str == NULL) {
        sprintf(err_string,"No strength specified for %s",uname[i]);
	if (put_error(err_string) == BREAK) {
	 return(BREAK);
	}
	goto gcname;
    }
    if (sscanf(str, "%f", &extinput[i]) != 1) {
	if (put_error("unrecognized value -- try again.") == BREAK) {
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
        if (put_error("Invalid pattern specification") == BREAK) {
	 return(BREAK);
	}
        goto again;
    }
    setinput();
    zarrays();

    cycle();
    return(CONTINUE);
}


newstart() {
	random_seed = rand();
	reset_system();
}

reset_system() {
    srand(random_seed);
    clear_display();
    zarrays();
    cs_update_display();
    return(CONTINUE);
}

init_system() {
    int get_unames(),test_pattern(),read_weights(),write_weights();

    epsilon_menu = NOMENU;

    install_command("network", define_network, GETMENU,(int *) NULL);
    install_command("weights", read_weights, GETMENU,(int *) NULL);
    install_command("cycle", cycle, BASEMENU,(int *) NULL);
    install_command("input", input, BASEMENU,(int *) NULL);
    install_command("test", test_pattern, BASEMENU,(int *) NULL);
    install_command("unames", get_unames, GETMENU,(int *) NULL);
    install_command("patterns", get_patterns, GETMENU,(int *) NULL);
    install_command("reset", reset_system, BASEMENU,(int *) NULL);
    install_command("newstart", newstart, BASEMENU,(int *) NULL);
    install_command("weights", write_weights, SAVEMENU,(int *) NULL);
    install_command("annealing", get_schedule, GETMENU,(int *) NULL);

    install_var("patno", Int,(int *) & patno, 0, 0, SETSVMENU);
    init_patterns();
    install_var("cycleno", Int,(int *) & cycleno, 0, 0, SETSVMENU);
    install_var("updateno", Int,(int *) & updateno, 0, 0, SETSVMENU);
    install_var("unitno", Int,(int *) & unitno, 0, 0, SETSVMENU);
    install_var("cuname", String,(int *) cuname, 0, 0, SETSVMENU);
    install_var("clamp", Int,(int *) & clamp, 0, 0, SETMODEMENU);
    install_var("nunits", Int,(int *) & nunits, 0, 0, SETCONFMENU);
    install_var("ninputs", Int,(int *) & ninputs, 0, 0, SETCONFMENU);
    install_var("estr", Float,(int *) & estr, 0, 0, SETPARAMMENU);
    install_var("istr", Float,(int *) & istr, 0, 0, SETPARAMMENU);
    install_var("kappa", Float,(int *) & kappa, 0, 0, SETPARAMMENU);
    install_var("boltzmann", Int, (int *) & boltzmann, 0, 0, 
    							SETMODEMENU);
    install_var("harmony", Int, (int *) & harmony, 0, 0, SETMODEMENU);
    install_var("temperature",Float, (int *) & temperature, 0, 0, 
    							SETSVMENU);
    install_var("goodness",Float, (int *) & goodness, 0, 0, SETSVMENU);
    install_var("ncycles", Int,(int *) & ncycles, 0, 0, SETPCMENU);
    install_var("nupdates", Int,(int *) & nupdates, 0, 0, SETPCMENU);
}

cs_update_display() {
	if (unitno < nunames) strcpy(cuname,uname[unitno]);
	update_display();
}
