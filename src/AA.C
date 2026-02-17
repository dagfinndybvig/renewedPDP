/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* file: aa.c

	Do the actual work for the aa program.
	
	First version implemented by Elliot Jaffe
	
	Date of last revision:  8-12-87/JLM

*/

#include "general.h"
#include "aa.h"
#include "variable.h"
#include "patterns.h"
#include "command.h"
#include <math.h>

char   *Prompt = "aa: ";
char   *Default_step_string = "pattern";

boolean System_Defined = FALSE;

/* parameters: */

float   estr = 0.15;		/* strength parameter */
float	istr = 0.15;
float   decay = 0.15;	 /* proportion of old activation lost */
float   lrate = 0.125;	   /* proportion of increment aquired */
float   pflip = 0.0;
float	ecrit = 0.001;

/* mostly program control variables: */

int     epochno = 0;
int	cycleno = 0;
int	patno = 0;
int     ncycles = 25;
int	nepochs = 1;
int     nunits = 0;
int	ninputs;
int	noutputs;
int     npatterns = 0;
int	tallflag = 0;

boolean	lflag = TRUE;

/* modes: */

boolean hebb = 0;
boolean linear = 0;
boolean bsb  = 0;
boolean self_connect = 0;


/* arrays (or more correctly pointers to arrays) */

float  **weight;		/* nunits X nunits */
float  *activation;		/* nunits */
float  *prioract;		/* nunits */
float  *netinput;		/* nunits */
float  *intinput;		/* nunits */
float  *error;			/* nunits */
float  *extinput;		/* nunits */

float   pss = 0.0;		/* pattern sum of squares */
float   tss,			/* total sum of squares */
	ndp,			/* normalized dot product */
	nvl,			/* normalized vector length */
	vcor;			/* vector correlation */

float   *sumv1;
float	*sumv2;
float	*sumv3;
int	sumlen;

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
    for (j = 0; j < len+1; j++) {
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

/* first get the number of units, then malloc all the necessary
   data structures.
*/

define_system() {
    int     i,j;

    if(nunits <= 0) {
	put_error("cannot initialize system without first defining nunits");
	return(FALSE);
    }
    ninputs = nunits;

    weight = ((float **) emalloc((unsigned)(sizeof(float*) * nunits)));
    install_var("weight", PVfloat,(int *) weight, nunits, nunits, 
		       SETWTMENU);
    for (i = 0 ; i < nunits; i++) {
	weight[i] = ((float *) emalloc((unsigned)(sizeof(float) * nunits)));
	for (j = 0; j < nunits; j++) {
	    weight[i][j] = 0.0;
	}
    }
    activation = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    install_var("activation", Vfloat,(int *) activation, 
						    nunits, 0, SETSVMENU);
    for (i = 0; i < nunits; i++)
	activation[i] = 0.0;

    prioract = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    install_var("prioract", Vfloat,(int *) prioract,
						    nunits, 0, SETSVMENU);
    for (i = 0; i < nunits; i++)
	prioract[i] = 0.0;

    netinput = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    for (i = 0; i < nunits; i++)
	netinput[i] = 0.0;

    intinput = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    install_var("intinput",Vfloat,(int *)intinput,nunits,0,SETSVMENU);
    for (i = 0; i < nunits; i++)
	intinput[i] = 0.0;

    error = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    install_var("error",Vfloat,(int *)error,nunits,0,SETSVMENU);
    for (i = 0; i < nunits; i++)
	error[i] = 0.0;

    extinput = (float *) emalloc((unsigned)(sizeof(float) * nunits));
    install_var("extinput",Vfloat,(int *) extinput,nunits,0,SETSVMENU);
    for (i = 0; i < nunits; i++)
	extinput[i] = 0.0;

    sumv1 = extinput;
    sumv2 = activation;
    sumv3 = intinput;
    sumlen = nunits;
    System_Defined = TRUE;

    wreset();

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
    if ((iop = fopen(str, "r")) == NULL) {
    	sprintf(err_string,"Cannot open %s.",str);
	return(put_error(err_string));
    }
    for (i = 0; i < nunits; i++) {
	for (j = 0; j < nunits; j++) {
	    (void) fscanf(iop, "%f", &weight[i][j]);
	}
	(void) fscanf(iop, "\n");
    }
    epochno = 0;
    ndp = 0;
    vcor = 0;
    nvl = 0;
    tss = 0;
    areset();
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
       return(CONTINUE);

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
    for (i = 0; i < nunits; i++) {
	for (j = 0; j < nunits; j++) {
	    fprintf(iop, "%6.3f", weight[i][j]);
	}
	fprintf(iop, "\n");
    }
    (void) fclose(iop);
    return(CONTINUE);
}


getinput() {
    register int    j;
    char   *str;

    if(! System_Defined)
      if(! define_system())
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

float
dotprod(v1,v2,len) float *v1, *v2; int len; {
	register int i;
	double dp = 0;
	double denom;
	denom = (double) len;

	for (i = 0; i < len; i++) {
		dp += (double) ((*v1++)*(*v2++));
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
	float denom,cv;
	float dp = 0.0;
	float l1 = 0.0;
	float l2 = 0.0;

	for (i = 0; i < len; i++,v1++,v2++) {
		dp += (*v1)*(*v2);
		l1 += (*v1)*(*v1);
		l2 += (*v2)*(*v2);
	}
	if (l1 == 0.0 || l2 == 0.0) return (0.0);
	denom = (float) sqrt( (double) (l1*l2));
	cv = dp/denom;
	return(cv);
}

float
veclen(v,len) float *v; int len; {
	int i;
	float denom;
	float vlen = 0;
	denom = (float) len;
	
	for (i = 0; i < len; i++,v++) {
		vlen += (*v)*(*v)/denom;
	}
	vlen = (float) sqrt((double) (vlen));
	return(vlen);
}

sumstats(level) int level; {
	ndp = dotprod(sumv1,sumv2,sumlen);
	nvl = veclen(sumv2,sumlen);
	vcor = veccor(sumv1,sumv2,sumlen);
	if (level) pss = sumsquares(sumv1,sumv3,sumlen);
}

cycle() {
    char *str;
    int iter;
    for (iter = 0; iter < ncycles; iter++) {
	cycleno++;
	getnet();
	if (update() == BREAK) return (BREAK);
	if (step_size == CYCLE) {
	    sumstats(0);
	    update_display();
	    if (single_flag) {
	    	if (contin_test() == BREAK) return (BREAK);
	    }
	}
	if (Interrupt) {
	    Interrupt_flag = 0;
	    sumstats(0);
	    update_display();
	    if (contin_test() == BREAK) return (BREAK);
	}
    }
    return(CONTINUE);
}

trial() {
    int br;
    if (patno < 0) cpname[0] = '\0';
    else strcpy(cpname,pname[patno]);
    areset();
    br = cycle();
    compute_error();
    sumstats(1);
    tss += pss;
    return(br);
}

getnet() {
    register int    i,j;

    for (i = 0; i < nunits; i++) { /*receiver*/
        intinput[i] = 0.0;
	for (j = 0; j < nunits; j++) { /*sender */
	    if ( (i == j) && !self_connect) continue;
	    intinput[i] += activation[j]*weight[i][j];
	}
    }
    for (i = 0; i < nunits; i++) {
	netinput[i] = istr*intinput[i] + estr*extinput[i];
    }
}

update() {
    float omd;
    float *np, *op, *pp;

    omd = (1 - decay);
    if (!linear) {
      for (op = activation, np = netinput, pp = prioract;
      				op < activation + nunits; op++,np++,pp++) {
	*pp = *op;
	if (*np > 0)
	    *op = omd * (*op) + *np * (1.0 - *op);
	else
	    *op = omd * (*op) + *np * (*op - (-1.0));
	if (*op > 1.0)
	    *op = 1.0;
	else if (*op < -1.0)
	    *op = -1.0;
      }
    }
    else {
      for (op = activation, np = netinput, pp = prioract;
                            op < activation + nunits; op++,np++,pp++) {
	*pp = *op;
	*op = omd * (*op) + *np;
        if (bsb) {
	  if (*op > 1.0)
	    *op = 1.0;
	  else if (*op < -1.0)
	    *op = -1.0;
	}
	else {
	  if (*op > 10.0 || *op < -10.0) {
	     get_command("Runaway activation!! Hit <cr> for command prompt: ");
	     return(BREAK);
	  }
	}
      }
    }
    return (CONTINUE);
}

compute_error() {
     int i;
  
     for (i = 0; i < nunits; i++) {
       error[i] = extinput[i] - intinput[i];
     }
}

change_weights() {
    register int i,j;

    /* The hebbian scheme is based on the notion that the
       pattern learned is the outer product of the input
       with itself */
       
    if (hebb) {
      for (i = 0; i < nunits; i++) {
	for (j = 0; j < nunits; j++) {
	  if ( (i == j) && !self_connect) continue;
	  weight[i][j] += lrate*extinput[i]*extinput[j];
	}
      }
    }
    else {
      for (i = 0; i < nunits; i++) {
	for (j = 0; j < nunits; j++) {
	  if ( (i == j) && !self_connect) continue;
	  weight[i][j] += lrate*error[i]*activation[j];
	}
      }
    }
}

areset() {
    register int    i;

    if(! System_Defined)
      if(! define_system())
       return;
    
    pss = ndp = vcor = nvl = 0.0;
    cycleno = 0;

    for (i = 0; i < nunits; i++) {
	intinput[i] = netinput[i] = activation[i] = error[i] = prioract[i] = 0;
    }
}

newstart() {
	random_seed = rand();
	wreset();
}

wreset() {
    register int    i,
                    j;
    
    if(! System_Defined)
      if(! define_system())
       return(BREAK);

    epochno = 0;
    pss = 0;
    tss = 0;
    cpname[0] = '\0';
    
    srand(random_seed);

    if(weight != NULL) {
	for(i = 0; i < nunits; i++)
	   for(j = 0; j < nunits; j++)
	     weight[i][j] = 0.0;
    }
    for (i = 0; i < nunits; i++) extinput[i] = 0.0;

    areset();
    update_display();
    return(CONTINUE);
}

distort(vect,pattern,len,amount) 
float *vect;
float *pattern;
int len;
float   amount;
{
    int    i;
    float   prop,val;

    for (i = 0; i < len; i++) {
	prop = (float) rnd();
	val = pattern[i];
	if (prop > amount)
	    vect[i] = val;
	else
	    vect[i] = 0.0 - val;
    }
}

strain() {
    return(train('s'));
}

ptrain() {
    return(train('p'));
}

train(c) char c; {
    int     t,i,old,npair,br;
    char    *str;

    if (!System_Defined)
	if (! define_system())
	    return(CONTINUE);

    for (t = 0; t < nepochs; t++) {
	if (!tallflag) epochno++;
	for (i = 0; i < npatterns; i++)
	    used[i] = i;
	if (c == 'p') {
	  for (i = 0; i < npatterns; i++) {
	    npair = rnd() * (npatterns - i) + i;
	    old = used[i];
	    used[i] = used[npair];
	    used[npair] = old;
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
	    distort(extinput, ipattern[patno], nunits, pflip);
	    if ((br = trial()) == BREAK) return(BREAK);
	    if(lflag) change_weights();
	    if ((lflag && step_size < PATTERN) || (step_size == PATTERN)) {
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
	if (tss < ecrit) break;
    }
    if (step_size == NEPOCHS) {
	  update_display();
    }
    return(CONTINUE);
}

comptest() {
    int     cl_start = 0;
    int	    cl_end = 0;
    int i;
    char   *str;
    int save_single;
    int save_step;

    if(! System_Defined)
      if(! define_system())
       return(BREAK);

    tss = 0.0;

    str = get_command("which pattern? ");
    if (str == NULL) return(CONTINUE);
    if((patno = get_pattern_number(str)) < 0) {
	return(put_error("Invalid pattern number."));
    }
    distort(extinput, ipattern[patno], nunits, 0.0);

    str = get_command("first element to clear? ");
    if (str == NULL) return(put_error("Must specify first element."));
    sscanf(str,"%d",&cl_start);
    if(cl_start >= nunits) {
      return(put_error("value must be from 0 to nunits - 1."));
    }
    str = get_command("last element? ");
    if (str == NULL) return(put_error("Must specify last element."));
    sscanf(str,"%d",&cl_end);
    if(cl_end >= nunits) {
      return(put_error("value must be from first to nunits - 1."));
    }
    for (i = cl_start; i <= cl_end; i++) {
	extinput[i] = 0.0;
    }
    /* next lines over-writes normal values with values 
       for the completed part of vector only */
    sumv1 = ipattern[patno]+cl_start;
    sumv2 = activation+cl_start;
    sumv3 = intinput+cl_start;
    sumlen = 1+cl_end - cl_start;
    save_single = single_flag; single_flag = 1;
    save_step = step_size;
    if (step_size > NCYCLES) {
    	step_size = NCYCLES;
    }
    trial();
    single_flag = save_single; step_size = save_step;
    update_display();
    /* resetting to normal values */
    sumv1 = extinput;
    sumv2 = activation;
    sumv3 = intinput;
    sumlen = nunits;
    return(CONTINUE);
}

test() {
    float  old_pflip;
    float *ivec;
    char   *str;
    int save_single;
    int save_step;
    
    tss = 0.0;

    if(! System_Defined)
      if(! define_system())
       return(CONTINUE);

    str = get_command
     ("test what (#N for pattern N, ?N to distort, L for last, E for enter)? ");
    if (str == NULL) return(CONTINUE);
    if(*str == '#') {
	if((patno = get_pattern_number(++str)) < 0) {
	   return(put_error("Invalid pattern specification."));
	}
        distort(extinput, ipattern[patno], nunits, 0.0);
    }
    else if (*str == '?') {
	if((patno = get_pattern_number(++str)) < 0) {
	   return(put_error("Invalid pattern specification."));
	}
        distort(extinput, ipattern[patno], nunits, pflip);
    }
    else if (*str == 'L') {
    	/* do nothing, leaving the last pattern in place */
    }
    else if (*str == 'E') {
	patno = -1;
	if ((ivec = readvec(" input ",nunits)) == (float *) NULL) 
		return(CONTINUE);
        distort(extinput, ivec, nunits, 0.0);
    }
    else {
    	return(put_error("Invalid input to the test commmand."));
    }
    save_single = single_flag; single_flag = 1;
    save_step = step_size; 
    if (step_size > NCYCLES) {
    	step_size = NCYCLES;
    }
    trial();
    single_flag = save_single; step_size = save_step;
    update_display();
    return(CONTINUE);
}

tall() {
    int save_lflag;
    int save_nepochs;
    int save_single_flag;
    int save_step_size;
    save_step_size = step_size;
    if (step_size > PATTERN) step_size = PATTERN;
    save_lflag = lflag; lflag = 0;
    save_nepochs = nepochs; nepochs = 1;
    save_single_flag = single_flag; 
    if (in_stream == stdin) single_flag = 1;
    tallflag = 1;
    train('s');
    tallflag = 0;
    single_flag = save_single_flag;
    lflag = save_lflag;
    nepochs = save_nepochs;
    step_size = save_step_size;
    return(CONTINUE);
}

make_patterns() {
    register int    i,
                    j;
    float   frac;
    char   *str;
    char    temp[20];

    if(! System_Defined)
      if(! define_system())
       return;

    str = get_command("How many patterns? ");
    if (str == NULL) return(CONTINUE);
    sscanf(str,"%d",&npatterns);
    str = get_command("make input + with probability: ");
    if (str == NULL) {
        return (put_error("Must give probability."));
    }
    sscanf(str,"%f",&frac);
    reset_patterns(!PAIRS);
    for (i = 0; i < npatterns; i++) {
        sprintf(temp,"%s%d","r",i);
	if (i == maxpatterns) {
	    enlarge_patterns(!PAIRS);
	}
	pname[i] = (char *) emalloc((unsigned) (strlen(temp) +1));
	strcpy(pname[i],temp);
	ipattern[i] = (float *) emalloc((unsigned)nunits*sizeof(float));
	for (j = 0; j < nunits; j++) {
	    if ((rnd()) < frac)
		ipattern[i][j] = 1.0;
	    else
		ipattern[i][j] = -1.0;
	}
    }
    change_variable_length("ipattern",npatterns,nunits);
    change_variable_length("pname",npatterns,0);
    clear_display();
    update_display();
    return(CONTINUE);
}

save_patterns() {
    register int    i,j;
    FILE * iop;
    char   *str;

    if(! System_Defined)
      if(! define_system())
       return(BREAK);

fnameagain:
    str = get_command("filename for patterns: ");
    if (str == NULL) return(CONTINUE);
    if ((iop = fopen(str, "r")) != NULL) {
        fclose(iop);
	get_command("File exists -- clobber? ");
	if (str == NULL || str[0] != 'y') 
		goto fnameagain;
    }
    if ((iop = fopen(str, "w")) == NULL) {
	return(put_error("cannot open output file"));
    }
    for (i = 0; i < npatterns; i++) {
       fprintf(iop,"%s ",pname[i]);
       for (j = 0; j < nunits; j++) {
	    fprintf(iop,"%f ",ipattern[i][j]);
       }
       fprintf(iop,"\n");
    }
    (void) fclose(iop);
    return(CONTINUE);
}

init_system() {

     int get_unames();

    install_command("strain", strain, BASEMENU,(int *) NULL);
    install_command("ptrain", ptrain, BASEMENU,(int *) NULL);
    install_command("tall", tall, BASEMENU,(int *) NULL);
    install_command("ctest", comptest, BASEMENU,(int *) NULL);
    install_command("test", test, BASEMENU,(int *) NULL);
    install_command("rpatterns", make_patterns, GETMENU,(int *) NULL);
    install_command("weights",get_weights, GETMENU, (int *) NULL);
    install_command("patterns",get_patterns, GETMENU,(int *) NULL);
    install_command("unames", get_unames, GETMENU,(int *) NULL);
    install_command("weights",save_weights, SAVEMENU, (int *) NULL);
    install_command("patterns", save_patterns, SAVEMENU,(int *) NULL);
    install_command("newstart", newstart, BASEMENU,(int *) NULL);
    install_command("reset", wreset, BASEMENU,(int *) NULL);

    install_var("linear", Int, (int *) & linear, 0, 0,SETMODEMENU);
    install_var("bsb", Int, (int *) & bsb, 0, 0, SETMODEMENU);
    install_var("hebb", Int, (int *) & hebb, 0, 0, SETMODEMENU);
    install_var("selfconnect", Int, (int *) & self_connect, 0, 0, 
    								SETMODEMENU);
    install_var("nunits", Int,(int *) & nunits, 0, 0, SETCONFMENU);
    install_var("lflag", Int, (int *) & lflag, 0, 0, SETPCMENU);
    install_var("estr", Float,(int *) & estr, 0, 0, SETPARAMMENU);
    install_var("istr", Float,(int *) & istr, 0, 0,  SETPARAMMENU);
    install_var("decay", Float,(int *) & decay, 0, 0, SETPARAMMENU);
    install_var("lrate", Float,(int *) & lrate, 0, 0, SETPARAMMENU);
    install_var("pflip", Float,(int *) & pflip, 0, 0, SETPARAMMENU);
    install_var("nepochs", Int,(int *) & nepochs, 0, 0, SETPCMENU);
    install_var("ncycles", Int,(int *) & ncycles, 0, 0, SETPCMENU);
    install_var("ecrit", Float, (int *)& ecrit,0,0,SETPCMENU);
    install_var("epochno", Int, (int *) & epochno, 0,0, SETSVMENU);
    install_var("patno", Int, (int *) & patno, 0,0, SETSVMENU);
    init_patterns();
    install_var("cycleno", Int, (int *) & cycleno, 0,0, SETSVMENU);
    install_var("tss", Float,(int *) & tss, 0, 0, SETSVMENU);
    install_var("pss", Float,(int *) & pss, 0, 0, SETSVMENU);
    install_var("ndp", Float, (int *) & ndp, 0, 0, SETSVMENU);
    install_var("nvl", Float, (int *) & nvl, 0, 0, SETSVMENU);
    install_var("vcor", Float, (int *) & vcor, 0, 0, SETSVMENU);

}
