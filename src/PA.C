/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* file: pa.c

	Do the actual work for the pa program.
	
	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

#include "general.h"
#include "pa.h"
#include "variable.h"
#include "weights.h"
#include "patterns.h"
#include "command.h"
#include <math.h>

char   *Prompt = "pa: ";
boolean System_Defined = FALSE;
char   *Default_step_string = "epoch";
boolean lflag = 1;
boolean linear = 0;
boolean	lt = 0;
boolean cs = 0;
boolean hebb = 0;
int     epochno = 0;
int     nepochs = 500;
int     patno = 0;
float	ndp = 0;
float	nvl = 0;
float	vcor = 0;
float   tss = 0.0;
float   pss = 0.0;
float   ecrit = 0.0;
float  *netinput = NULL;
float  *output = NULL;
float  *error = NULL;
float  *input = NULL;
float  *target = NULL;
float	noise = 0;
float   temp = 15.0;
int	tallflag = 0;


extern int read_weights();
extern int write_weights();

float *
readvec(pstr,len) char *pstr; int len; {
    int j;
    float *tvec;
    char *str;
    char tstr[60];
    
    if (pstr == NULL) {
        tvec = (float *) emalloc((unsigned)(sizeof(float)*len));
	for (j = 0; j < len; j++) {
	    tvec[j] = 0.0;
	}
	return(tvec);
    }
    sprintf(tstr,"give %selements:  ",pstr);
    tvec = (float *) emalloc((unsigned)(sizeof(float)*len));
    for (j = 0; j < len; j++) {
	tvec[j] = 0.0;
    }
    for (j = 0; j <= len; j++) {
        str = get_command(tstr);
	if (str == NULL || strcmp(str,"end") == 0) {
	    if (j) return(tvec); else return (NULL);
	}
	if (strcmp("+",str) == 0) tvec[j] = 1.0;
	else if (strcmp("-",str) == 0) tvec[j] = -1.0;
	else if (strcmp(".",str) == 0) tvec[j] = 0.0;
	else sscanf(str,"%f",&tvec[j]);
    }
    return(tvec);
}

float *
get_vec() {
    char * str;
    int j;
    str = 
      get_command("vector (iN for ipattern, tN for tpattern, E for enter): ");
    if (str == NULL) return(NULL);
    if(*str == 'i') {
	if((patno = get_pattern_number(++str)) < 0) {
	    put_error("Invalid pattern specification.");
	    return(NULL);
	}
        return(ipattern[patno]);
    }
    else if(*str == 't') {
	if((patno = get_pattern_number(++str)) < 0) {
	    put_error("Invalid pattern specification.");
	    return(NULL);
	}
        return(tpattern[patno]);
    }
    else return(readvec(" ",nunits));
}

float
dotprod(v1,v2,len) float *v1, *v2; int len; {
	register int i;
	double dp = 0;
	double denom;
	denom = (double) len;
	if (denom == 0) return(0.0);
	for (i = 0; i < len; i++,v1++,v2++) {
		dp += (double) ((*v1)*(*v2));
	}
	dp /= denom;
	return(dp);
}

float
sumsquares(v1,v2,len) float *v1, *v2; int len; {
	register int i;
	double ss = 0;

	for (i = 0; i < len; i++,v1++,v2++) {
		ss += (double)((*v1 - *v2) * (*v1 - *v2));
	}
	return(ss);
}

/* the following function computes the vector correlation, or the
   cosine of the angle between v1 and v2 */

float
veccor(v1,v2,len) float *v1, *v2; int len; {
	register int i;
	double denom;
	double dp = 0.0;
	double l1 = 0.0;
	double l2 = 0.0;

	for (i = 0; i < len; i++,v1++,v2++) {
		dp += (double) (*v1)*(*v2);
		l1 += (double) (*v1)*(*v1);
		l2 += (double) (*v2)*(*v2);
	}
	if (l1 == 0.0 || l2 == 0.0) return (0.0);
	dp /= sqrt(l1*l2);
	return(dp);
}

float
veclen(v,len) float *v; int len; {
	int i;
	double denom;
	double vl = 0;
	denom = (double) len;
	if (denom == 0) {
	    return(0.0);
	}
	for (i = 0; i < len; i++,v++) {
		vl += (*v)*(*v)/denom;
	}
	vl = sqrt((vl));
	return(vl);
}

distort(vect,pattern,len,amount) 
float *vect;
float *pattern;
int len;
float   amount;
{
    int    i;
    float   rval,val;

    for (i = 0; i < len; i++) {
	rval = (float) (1.0 - 2.0*rnd());
	*vect++ = *pattern++ + rval*amount;
    }
}

init_system() {
    int     strain (), ptrain (), tall (), get_unames(),
            test_pattern (), reset_weights(),newstart();
    int change_lrate();

    lrate = 2.0;
    epsilon_menu = NOMENU;
    install_var("lflag", Int,(int *) & lflag, 0, 0, SETPCMENU);

    install_command("strain", strain, BASEMENU,(int *) NULL);
    install_command("ptrain", ptrain, BASEMENU,(int *) NULL);
    install_command("tall", tall, BASEMENU,(int *) NULL);
    install_command("test", test_pattern, BASEMENU,(int *) NULL);
    install_command("reset",reset_weights,BASEMENU,(int *)NULL);
    install_command("newstart",newstart,BASEMENU,(int *)NULL);
    install_command("patterns", get_pattern_pairs, 
			   			GETMENU,(int *) NULL);
    install_command("unames", get_unames, GETMENU,(int *) NULL);
    install_var("nepochs", Int,(int *) & nepochs, 0, 0, SETPCMENU);
    install_command("lrate", change_lrate, SETPARAMMENU, (int *) NULL);
    install_var("lrate", Float,(int *) & lrate, 0, 0, NOMENU);
    install_var("ecrit", Float, (int *)& ecrit,0,0,SETPCMENU);
    install_var("noise", Float, (int *)&noise,0,0,SETPARAMMENU);
    install_var("linear", Int,(int *) &linear,0,0,SETMODEMENU);
    install_var("temp", Float, (int *)&temp,0,0,SETPARAMMENU);
    install_var("lt", Int,(int *) &lt,0,0,SETMODEMENU);
    install_var("cs", Int,(int *) &cs,0,0,SETMODEMENU);
    install_var("hebb", Int,(int *) &hebb,0,0,SETMODEMENU);
    install_var("epochno", Int,(int *) & epochno, 0, 0, SETSVMENU);
    install_var("patno", Int,(int *) & patno, 0, 0, SETSVMENU);
    init_pattern_pairs();
    install_var("tss", Float,(int *) & tss, 0, 0, SETSVMENU);
    install_var("pss", Float,(int *) & pss, 0, 0, SETSVMENU);
    install_var("ndp", Float,(int *) & ndp, 0, 0, SETSVMENU);
    install_var("vcor", Float,(int *) & vcor, 0, 0, SETSVMENU);
    install_var("nvl", Float,(int *) & nvl, 0, 0, SETSVMENU);
    init_weights();
}

define_system() {
    register int    i,j;

    if (!nunits) {
	put_error("cannot init pa system, nunits not defined");
	return(FALSE);
    }
    else
	if (!noutputs) {
	    put_error("cannot init pa system, noutputs not defined");
	    return(FALSE);
	}
    else
	if (!ninputs) {
	    put_error("cannot init pa system, ninputs not defined");
	    return(FALSE);
	}
    netinput = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    install_var("netinput", Vfloat,(int *) netinput,
	    nunits, 0, SETSVMENU);
    for (i = 0; i < nunits; i++)
	netinput[i] = 0.0;

    output = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    install_var("output", Vfloat,(int *) & output[ninputs],
	    noutputs, 0, SETSVMENU);
    for (i = 0; i < nunits; i++)
	output[i] = 0.0;

    error = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    install_var("error", Vfloat,(int *) & error[ninputs], 
		        noutputs, 0, SETSVMENU);
    for (i = 0; i < nunits; i++)
	error[i] = 0.0;

    target = (float *) emalloc((unsigned)(sizeof(float) * noutputs));
    install_var("target", Vfloat,(int *) target, noutputs, 0,
		       SETSVMENU);
    for (i = 0; i < noutputs; i++)
	target[i] = 0.0;

    input = (float *) emalloc((unsigned)(sizeof(float) * ninputs));
    install_var("input", Vfloat,(int *) input, ninputs, 0, SETSVMENU);
    
    for (i = 0; i < ninputs; i++)
	input[i] = 0.0;

    System_Defined = TRUE;
    return(TRUE);
}


float  logistic (x)
float  x;
{
    x /= temp;
    if (x > 11.5129)
	return(.99999);
      else
	if (x < -11.5129)
	    return(.00001);
    else
       return(1.0 / (1.0 + (float) exp( (double) ((-1.0) * x))));
}

probability(val)
float  val;
{
    return((rnd() < val) ? 1 : 0);
}


compute_output() {
    register int    i,j,sender,num;

    for (i = ninputs; i < nunits; i++) {/* ranges over output units */
	netinput[i] = bias[i];
	sender = first_weight_to[i];
	num = num_weights_to[i];
	for (j = 0; j < num; j++) { /* ranges over input units */
	    netinput[i] += output[sender++]*weight[i][j];
	}
	if (linear) {
	  output[i] = netinput[i];
	}
	else if (lt) {
	  output[i] = (float) (netinput[i] > 0 ? 1.0 : 0.0 );
	}
	else if	(cs) {
	  output[i] =  logistic(netinput[i]);
	}
	else { /* default, stochastic mode */
	  output[i] = (float)probability((float)logistic(netinput[i]));
	}
    }
}

compute_error() {
    register int    i,j;

    for (i = ninputs, j = 0; i < nunits; j++, i++) {
	error[i] = target[j] - output[i];
    }
}

change_weights() {
    register int    i,j,ti,sender,num;

    if (hebb) {
      for (i = ninputs,ti = 0; i < nunits; i++,ti++) {
        output[i] = target[ti];
	sender = first_weight_to[i];
	num = num_weights_to[i];
	for (j = 0; j < num; j++) {
	     weight[i][j] +=
			epsilon[i][j]*output[i]*output[sender++];
	}
	bias[i] += bepsilon[i]*output[i];
      }
    }
    else { /* delta rule, by default */
      for (i = ninputs; i < nunits; i++) {
	sender = first_weight_to[i];
	num = num_weights_to[i];
	for (j = 0; j < num; j++) {
	     weight[i][j] +=
			epsilon[i][j]*error[i]*output[sender++];
	}
	bias[i] += bepsilon[i]*error[i];
      }
    }
}

constrain_weights() {
}

setinput() {
    register int    i;

    for (i = 0; i < ninputs; i++) {
	    output[i] = input[i];
    }
    if (patno < 0) cpname[0] = '\0';
    else strcpy(cpname,pname[patno]);
}

trial() {
    setinput();
    compute_output();
    compute_error();
    sumstats();
}

sumstats() {

    pss  =  (float) sumsquares(target,&output[ninputs],noutputs);
    vcor =  (float) veccor(target,&output[ninputs],noutputs);
    nvl  =  (float) veclen(&output[ninputs],noutputs);
    ndp  =  (float) dotprod(target,&output[ninputs],noutputs);
    tss += pss;
}

ptrain() {
  train('p');
}

strain() {
  train('s');
}

train(c) char c; {
    int     t,i,old,npat;
    char    *str;

    if (!System_Defined)
	if (!define_system())
	    return;

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
	tss = 0.0;
	for (i = 0; i < npatterns; i++) {
	    if (Interrupt) {
		Interrupt_flag = 0;
		update_display();
		if (contin_test() == BREAK) return(BREAK);
	    }
	    patno = used[i];
	    distort(input,ipattern[patno],ninputs,noise);
	    distort(target,tpattern[patno],noutputs,noise);
	    trial();
	    /* the && lflag insures that we do not get a redundant
	       display update if change_weights is not going to be
	       called */
	    if (step_size == CYCLE && lflag) {
		update_display();
	        if (single_flag) {
		   if (contin_test() == BREAK) return(BREAK);
		}
	    }
	    if (lflag) change_weights();
	    if (step_size <= PATTERN) {
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
	if (tss < ecrit)
	    break;
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
  save_step_size = step_size; if (step_size > PATTERN) step_size = PATTERN;
  tallflag = 1;
  train('s');
  tallflag = 0;
  lflag = save_lflag;
  nepochs = save_nepochs;
  single_flag = save_single_flag;
  step_size = save_step_size;
}
  
test_pattern() {
    char   *str;
    float *ivec, *tvec;
    float tmp_noise;

    if(! System_Defined)
      if(! define_system())
       return(CONTINUE);

    str = get_command("input (#N, ?N, E for enter): ");
    if (str == NULL) return(CONTINUE);
    if(*str == '#' || *str == '?') {
	if((patno = get_pattern_number(str+1)) < 0) {
	   return(put_error("Invalid pattern specification."));
	}
	tmp_noise = (float) (*str = '#' ? 0.0 : noise );
        distort(input, ipattern[patno], ninputs, tmp_noise);
    }
    else {
	patno = -1;
	if ((ivec = readvec(" input ",ninputs)) == (float *) NULL) 
		return(CONTINUE);
        distort(input, ivec, ninputs, 0.0);
    }
    str = get_command("target (#N, ?N, E for enter): ");
    if (str == NULL) {
	tvec = readvec(" target ",noutputs);
    }
    else if(*str == '#' || *str == '?') {
	if((patno = get_pattern_number(str+1)) < 0) {
	   return(put_error("Invalid pattern specification."));
	}
	tmp_noise = (float) (*str = '#' ? 0.0 : noise );
        distort(target, tpattern[patno], noutputs, tmp_noise);
    } 
    else {
	if ((tvec = readvec(" target ",noutputs)) == (float *) NULL) 
		return(CONTINUE);
        distort(target, tvec, noutputs, 0.0);
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
    register int    i,j,end;
    
    epochno = 0;
    tss = 0.0;
    pss = 0.0;
    patno = 0;
    ndp = vcor = nvl = 0.0;
    cpname[0] = '\0';
    
    srand(random_seed);

    if (!System_Defined)
	if (!define_system())
	    return;

    for (j = ninputs; j < nunits; j++) {
	for (i = first_weight_to[j], end = i + num_weights_to[j];
	     i < end; i++) {
		weight[j][i] = 0.0;
	}
	bias[j] = 0.0;
    }
    for (i = 0; i < ninputs; i++) {
      input[i] = 0.0;
    }
    for (i = 0; i < noutputs; i++) {
      target[i] = 0.0;
    }
    for (i = 0; i < nunits; i++) {
      output[i] = error[i] = 0.0;
    }
    update_display();
}

init_weights() {
    install_command("network", define_network, GETMENU,(int *) NULL);
    install_command("weights", read_weights, GETMENU,(int *) NULL);
    install_command("weights", write_weights, SAVEMENU,(int *) NULL);
    install_var("nunits", Int,(int *) & nunits, 0, 0, SETCONFMENU);
    install_var("ninputs", Int,(int *) & ninputs, 0, 0, SETCONFMENU);
    install_var("noutputs", Int,(int *) & noutputs, 0, 0, SETCONFMENU);
}
