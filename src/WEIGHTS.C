/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* file: weights.c

	read in network descriptions, and set up constraints.
	
	First version implemented by Elliot Jaffe.
	
	Date of last revision: 8-12-87/JLM.
*/

/*LINTLIBRARY*/

/* the following is the form for network description files.

definitions:
nunits <int>
ninputs <int>
noutputs <int>
maxconstraints <int>
constraints:
<char> <float> or <char> [random positive negative linked]
...
end
network:
<strings of . and chars as defined in definitions:>
end
biases:
<a single line of . and chars as biases for units>
end
sigmas:
<a single line of .'s and chars specifying sigmas -- harmony theory only>
end
<EOF>
*/


#include "general.h"
#include "command.h"
#include "weights.h"
#include "variable.h"

float **weight = NULL;
char   **wchar;			/* pointers to vectors of chars
					   that are used in resetting weights*/
float  *bias = NULL;
char   *bchar;				/* like wchar */
float **epsilon;
float  *bepsilon = NULL;	/* thresh epsilon array */
float  **wed = NULL;
float  *bed = NULL;
float  *sigma = NULL;		/* strength parameter for knowledge atoms */

struct constants    constants[26];

float **positive_constraints;
float **negative_constraints;
 /* Array of struct constraint, for keeping links together */
struct constraint  *constraints = NULL;

float   lrate = 0.5;

float   wrange = 1;

int     nunits = 0;
int     ninputs = 0;
int     noutputs = 0;
int	maxpos = MAXCONSTRAINTS;
int	maxneg = MAXCONSTRAINTS;
int     nlinks = 0;
static  nposconstr = 0;
static  nnegconstr = 0;
int     epsilon_menu = SETWTMENU;
char    net_descr_name[BUFSIZ];

int bp;	/* TRUE if program is bp */

# define ENLARGE_POS -1
# define ENLARGE_NEG -2

define_bp_network() {
	bp = 1;
	define_net();
}

define_network() {
	bp = 0;
	define_net();
}

define_net() {
    char   *sp;
    char    string[BUFSIZ];
    FILE * sv_instream;
    struct Variable *lookup_var ();
    int     i;
    boolean defined_weights = FALSE;

    sv_instream = in_stream;
    sp = get_command("filename for network description: ");
    if ( sp == NULL) return(CONTINUE);
    strcpy(net_descr_name,sp);

    if ((in_stream = fopen(sp, "r")) == NULL) {
	in_stream = sv_instream;
	return(put_error("Can't open network file."));
    }

    nlinks = 0;

    for (i = 0; i < 26; i++) {
	constants[i].random = FALSE;
	constants[i].positive = FALSE;
	constants[i].negative = FALSE;
	constants[i].link = FALSE;
	constants[i].value = 0.0;
    }

    constants['r' - 'a'].random = TRUE;
    constants['p' - 'a'].random = TRUE;
    constants['p' - 'a'].positive = TRUE;
    constants['n' - 'a'].random = TRUE;
    constants['n' - 'a'].negative = TRUE;

    while (fscanf(in_stream, "%s", string) != EOF) {
	if (!strcmp(string, "definitions:")) {
	    if (read_definitions() == BREAK) {
	        fclose(in_stream); in_stream = sv_instream; 
		return(BREAK);
	    }
	}
	else
	    if (!strcmp(string, "constraints:")) {
		if (read_constraints(constants) == BREAK) {
	        	fclose(in_stream); in_stream = sv_instream; 
			return(BREAK);
		}
	    }
	else
	    if (!strcmp(string, "network:")) {
		defined_weights = read_network(constants);
		if (!defined_weights) {
		 if (put_error(err_string) == BREAK) {
	        	fclose(in_stream); in_stream = sv_instream; 
		 	return(BREAK);
		 }
		}
	    }
	else
	    if (!strcmp(string, "biases:")) {
		if (read_biases(constants) == BREAK) {
	        	fclose(in_stream); in_stream = sv_instream; 
			return(BREAK);
		}
	    }
	else
	    if (!strcmp(string, "sigmas:")) {
		if (read_sigmas(constants) == BREAK) {
	        	fclose(in_stream); in_stream = sv_instream; 
			return(BREAK);
		}
	    }
	else
	    if (!strcmp(string, "end")) {
	    /* just skip over it */
	    }
	else {
	    sprintf(err_string,
	      "error reading network file: I don't understand %s\n",string);
	    if (put_error(err_string) == BREAK) {
        	fclose(in_stream); in_stream = sv_instream; 
	    	return(BREAK);
	    }
	}
    }
    fclose(in_stream);
    in_stream = sv_instream;
    if (nlinks)
	constrain_weights();
    return(CONTINUE);
}

read_definitions() {
    char    string[BUFSIZ];
    struct Variable *varp,
                   *lookup_var ();

    while (fscanf(in_stream, "%s", string) != EOF) {
	if (!strcmp(string, "end"))
	    return(CONTINUE);
	if ((varp = lookup_var(string)) != NULL) {
	    change_variable(string,(int *) varp);
	}
	else {
	    sprintf(err_string,
	    	"Error: unknown variable in network file, %s\n", string);
	    return(put_error(err_string));
	}
    }
}

read_network(con)
struct constants   *con;
{
    int     i,r,s,block,since_first,last_weight_to,tempint;
    int	    rstart,rnum,rend,sstart,snum,send,con_index;
    char    ch,all_ch,*strp;
    char    string[BUFSIZ];
    int	    needline = 1;
    float   *tmp; char *ctmp;

    (void) srand(random_seed);
    weight = ((float **)  emalloc((unsigned int)(sizeof(float *) * nunits)));
    
    epsilon = ((float **) emalloc((unsigned int)(sizeof(float *)) * nunits));
    
    wchar = ((char **) emalloc((unsigned int)(sizeof(char *) * nunits)));

    first_weight_to = (int *) emalloc((unsigned int)(sizeof(int) * nunits));
    for (r = 0; r < nunits; r++)
	first_weight_to[r] = nunits;
    num_weights_to = (int *) emalloc((unsigned int)(sizeof(int) * nunits));
    for (r = 0; r < nunits; r++)
	num_weights_to[r] = 0;

    install_var("weight",PVweight,(int *) weight,nunits,nunits,
							SETWTMENU);
    install_var("epsilon", PVweight,(int *) epsilon, nunits, nunits, 
		       					epsilon_menu);
    if (bp) {
    	wed = ((float **) emalloc((unsigned int)(sizeof(float *) * nunits)));
    	install_var("wed",PVweight,(int *) wed,nunits,nunits,
							SETSVMENU);
    }
    
    rstart = 0; rend = nunits -1; sstart = 0; send = nunits -1;
    for (block = 0; ; block++) {
gbagain:
      if (fscanf(in_stream,"%s",string) == EOF) {
	    sprintf(err_string,"error in network description");
	    return(FALSE);
      }
      if (strcmp("end",string) == 0) {
      	if (block) return(TRUE);
	else {
	 sprintf(err_string,"error in network description");
	}
	return(FALSE);
      }
      all_ch = '\0';
      if (string[0] == '%') {
        fscanf(in_stream,"%d%d%d%d",&rstart,&rnum,&sstart,&snum);
	rend = rstart + rnum -1;
	send = sstart + snum -1;
	if (string[1]) {
	    all_ch = string[1];
	}
      }
      else {
       if (!block) {
	needline = 0;
       }
       else {
	    sprintf(err_string,"error in network description");
	    return(FALSE);
       }
      }
      for (r = rstart; r <= rend; r++) {
	  if (!all_ch) {
	    if (needline) {
             if (fscanf(in_stream,"%s",string) == EOF) {
	       sprintf(err_string,"not enough units in network description");
	       return(FALSE);
	     }
	    }
	    else needline = 1;
	  }
	  else {
	    for (s = 0; s < snum; s++) string[s] = all_ch;
	    string[s] = '\0';
	  }
	  first_weight_to[r] = sstart;
	  last_weight_to = send;	  
	  num_weights_to[r] = 1 + last_weight_to - first_weight_to[r];
	  weight[r] = ((float *)
	      emalloc ((unsigned int)(sizeof(float) * num_weights_to[r])));
	  epsilon[r] = ((float *)
	      emalloc ((unsigned int)(sizeof(float) * num_weights_to[r])));
	  wchar[r] = ((char *)
	      emalloc ((unsigned int)(sizeof(char) * num_weights_to[r])));
	  if (bp) {
	  wed[r] = ((float *)
	      emalloc ((unsigned int)(sizeof(float) * num_weights_to[r])));
	  }
	  for(s = 0; s < num_weights_to[r]; s++) {
	     weight[r][s] = 0.0;
	     epsilon[r][s] = 0.0;
	     wchar[r][s] = '.';
	     if (bp) wed[r][s] = 0.0;
	  }
	  for (strp = string,s = sstart,since_first = 0; s <= send; s++) {
				/* loop over the from units */
	    ch = *strp++;
	    wchar[r][since_first] = ch;
	    if (ch == '.') {
		since_first++;
	    }
	    else {
	    /* first check if this is realy a character */
		if (!isalpha(ch)) {
		    sprintf(err_string,"non_alpha character in network");
		    return(FALSE);
		}


	    /* upper case means this weight is non-changable */
		if (isupper(ch)) {
		/* make it lower case */
		    ch = tolower(ch);
		    epsilon[r][since_first] = 0;
		}
		else {
		    epsilon[r][since_first] = lrate;
		}

	    /* now set up the char based on the stored con definitions */
		if (con[ch - 'a'].random) {
		    if (con[ch - 'a'].positive) {
			if (nposconstr >= maxpos) {
			    enlarge_constraints(ENLARGE_POS);    
			}
			weight[r][since_first] = wrange * rnd();
			positive_constraints[nposconstr++] = 
			     &weight[r][since_first];
		    }
		    else
			if (con[ch - 'a'].negative) {
			    if (nnegconstr >= maxneg){
			    	enlarge_constraints(ENLARGE_NEG);
			    }
			    weight[r][since_first] = 
			         wrange * (rnd() - 1);
			    negative_constraints[nnegconstr++] = 
			         &weight[r][since_first];
			}
		    else
			weight[r][since_first] = wrange * (rnd() -.5);
		}
		else {
		    weight[r][since_first] = con[ch - 'a'].value;
		}
		if (con[ch - 'a'].link) {
		    con_index = (con[ch - 'a'].link - 1);
		    if (constraints[con_index].num >= constraints[con_index].max) {
			    enlarge_constraints(con_index);
		    }
		    
		    tempint = constraints[con_index].num;
		    constraints[con_index].cvec[tempint] 
		          = &weight[r][since_first];

		    if (bp) {
			constraints[con_index].ivec[tempint] 
		          = &wed[r][since_first];
		    }

	            tempint = constraints[con_index].num + 1;
		    constraints[con_index].num = tempint;
		    /* this kludge (tempint) is for the MS compiler */
		}
	    since_first++;
	    }
	  }
      }
    }
}

read_biases(con)
struct constants   *con;
{
    int     j,rstart,rend,rnum,block,con_index,tempint;
    char    ch,all_ch,*strp;
    char    string[BUFSIZ];

    bias = (float *) emalloc((unsigned int)(sizeof(float) * nunits));
    install_var("bias", Vfloat,(int *) bias, nunits, 0, SETWTMENU);

    bepsilon = (float *) emalloc((unsigned int)(sizeof(float) * nunits));
    install_var("bepsilon", Vfloat,(int *) bepsilon, nunits, 0, 
    							epsilon_menu);
    bchar = (char *) emalloc((unsigned int)(sizeof(char) * nunits));

    if (bp) {
    	bed = (float *) emalloc((unsigned int)(sizeof(float) * nunits));
        install_var("bed", Vfloat,(int *) bed,nunits,0,SETSVMENU);
    }
    
    for (j = 0; j < nunits; j++) {
	bias[j] = 0.0;
	bepsilon[j] = 0;
	bchar[j] = '.';
	if (bp) bed[j] = 0.0;
    }

    rstart = 0; rend = nunits -1;
  for (block = 0; ; block++) {
gtagain:
    if (fscanf(in_stream,"%s",string) == EOF) {
	return(put_error("problem in bias description"));
    }
    if (strcmp(string,"end") == 0) {
        if (block) return (CONTINUE);
        else return(put_error("problem in bias description"));
    }
    if (string[0] == '%') {
        fscanf(in_stream,"%d%d",&rstart,&rnum);
	rend = rstart + rnum -1;
	if (string[1] != '\0') {
	    all_ch = string[1];
	    for (j = 0; j < rnum; j++) {
		string[j] = all_ch;
	    }
	    string[j] = '\0';
	}
	else goto gtagain;
    }
    for (strp = string, j = rstart; j <= rend; j++, strp++) {
	ch = *strp;
	bchar[j] = ch;
	if (ch == '.') {
	    bias[j] = 0;
	    bepsilon[j] = 0;
	}
	else {
	/* first check if this is realy a character */
	    if (!isalpha(ch)) {
		return(put_error("non_alpha character in bias"));
	    }

	/* upper case means this weight is non-changable */
	    if (isupper(ch)) {
	    /* make it lower case */
		ch = tolower(ch);
		bepsilon[j] = 0;
	    }
	    else {
		bepsilon[j] = lrate;
	    }

	    /* now set up the char based on the stored con definitions */
	    if (con[ch - 'a'].random) {
		    if (con[ch - 'a'].positive) {
			bias[j] = wrange * rnd();
			if (nposconstr >= maxpos) {
			    enlarge_constraints(ENLARGE_POS);    
			}
			positive_constraints[nposconstr++] = &bias[j];
		    }
		    else
			if (con[ch - 'a'].negative) {
			    bias[j] = wrange * (rnd() - 1);
			    if (nnegconstr >= maxneg){
			    	enlarge_constraints(ENLARGE_NEG);
			    }
			    negative_constraints[nnegconstr++] = &bias[j];
			}
		    else
			bias[j] = wrange * (rnd() -.5);
	    }
	    else {
		    bias[j] = con[ch - 'a'].value;
	    }
	    if (con[ch - 'a'].link) {
		con_index = (con[ch - 'a'].link - 1);
		if (constraints[con_index].num >= constraints[con_index].max){
		    enlarge_constraints(con_index);
	        }
		tempint = constraints[con_index].num;
		constraints[con_index].cvec[tempint] = &bias[j];
		if (bp) constraints[con_index].ivec[tempint] = &bed[j];
		constraints[con_index].num++;
	    }
	}
    }
  }
}

read_sigmas(con) struct constants   *con; {
    int     j;
    char    ch, all_ch, *strp;
    char    string[BUFSIZ];
    int	    rstart, rend, rnum, block;

    sigma = (float *) emalloc((unsigned int)(sizeof(float) * nunits));
    for (j = 0; j < nunits; j++) {
      sigma[j] = 1.0; 		/* default sigma is 1.0 */
    }
    install_var("sigma", Vfloat,(int *) sigma, nunits, 0, 
		       SETWTMENU);
    rstart = 0; rend = nunits -1;
  for (block = 0; ; block++) {
gsagain:      
    if (fscanf(in_stream, "%s", string) == EOF) {
	return(put_error("problem in sigma description"));
    }
    if (strcmp(string,"end") == 0) {
        if (block) return (CONTINUE);
        else return(put_error("problem in sigma description"));
    }
    if (string[0] == '%') {
        fscanf(in_stream,"%d%d",&rstart,&rnum);
	rend = rstart + rnum -1;
	if (string[1] != '\0') {
	    all_ch = string[1];
	    for (j = 0; j < rnum; j++) {
		string[j] = all_ch;
	    }
	    string[j] = '\0';
	}
	else goto gsagain;
    }
    for (strp = string, j = rstart; j <= rend; j++, strp++) {
	ch = *strp;
	if (ch == '.') {
	  sigma[j] = 1.0;
	}
	else {
	/* first check if this is really a character */
	    if (!isalpha(ch)) {
		return(put_error("non_alpha character in bias"));
	    }
	    if (isupper(ch)) {
	    /* make it lower case */
		ch = tolower(ch);
	    }
	    sigma[j] = con[ch - 'a'].value;
	    if (sigma[j] < 0) {
	      return(put_error("can't set sigma less than 0!"));
	    }
	}
    }
  }
}

read_constraints(con)
struct constants   *con;
{
    char    ch;
    float   flt;
    int     isflt;
    char    string[BUFSIZ];
    char    str[5][30];
    int     i,j,ch_ind;
    int	    nstr;

    while (fgets(string, BUFSIZ, in_stream) != NULL) {
    	if (string[0] == NULL || string[0] == '\n') {
	    if (fgets(string, BUFSIZ, in_stream) == NULL) {
	    	break;
	    }
	}
	if (strncmp(string,"end",3) == 0) break;

	ch = '\0';

	for (i = 0; i < 5; i++) str[i][0] = '\0';

	(void) sscanf(string, "%c %s %s %s %s %s", 
	              &ch, str[0], str[1], str[2], str[3], str[4]);
	ch = (isupper(ch)) ? tolower(ch) : ch;
	ch_ind = ch - 'a';
	con[ch_ind].random = con[ch_ind].positive = 
	    con[ch_ind].negative = con[ch_ind].link = FALSE;
	con[ch_ind].value = 0.0;
	for (i = 0; (i < 5) && (str[i][0] != '\0'); i++) {
	    if ( (isflt = sscanf(str[i],"%f",&flt)) == 1) {
		con[ch_ind].value = flt;
	    }
	    else
		if (startsame(str[i], "random"))
		    con[ch_ind].random = TRUE;
	    else
		if (startsame(str[i], "positive"))
		    con[ch_ind].positive = TRUE;
	    else
		if (startsame(str[i], "negative"))
		    con[ch_ind].negative = TRUE;
	    else
		if (startsame(str[i], "linked"))
		    con[ch_ind].link = ++nlinks;
	    else {
		sprintf(err_string,
		  "unknown type for constant %c, %s\n", ch, str[i]);
		if (put_error(err_string) == BREAK) {
			return(BREAK);
		}
	    }
	}
    }
    if (nlinks) {
	constraints = (struct constraint   *) 
	  emalloc ((unsigned int)(sizeof (struct constraint) * (nlinks + 1)));
	for (i = 0; i < nlinks; i++) {
	    constraints[i].num = 0;
	    constraints[i].max = MAXCONSTRAINTS;
	    constraints[i].cvec = ((float **) 
	        emalloc((unsigned int)(sizeof(float *) * MAXCONSTRAINTS)));
	    constraints[i].ivec = ((float **) 
	        emalloc((unsigned int)(sizeof(float *) * MAXCONSTRAINTS)));
	    for (j = 0; j < constraints[i].max; j++) {
		constraints[i].cvec[j] = NULL;
		constraints[i].ivec[j] = NULL;
	    }
	}
    }
    else {
	constraints = NULL;
    }
    positive_constraints = ((float **) 
    		emalloc((unsigned int)(sizeof(float *) * MAXCONSTRAINTS)));
    for (i = 0; i < MAXCONSTRAINTS; i++)
	positive_constraints[i] = NULL;
    negative_constraints = ((float **) 
    		emalloc((unsigned int)(sizeof(float *) * MAXCONSTRAINTS)));
    for (i = 0; i < MAXCONSTRAINTS; i++)
	negative_constraints[i] = NULL;
    return(CONTINUE);
}

change_lrate() {
    struct Variable *varp;
    int     i,
            j;

    if ((varp = lookup_var("lrate")) != NULL) {
	change_variable("lrate",(int *) varp);
    }
    else {
	return(put_error("BIG PROBLEM: lrate is not defined"));
    }

    if (epsilon != NULL) {
	for (i = 0; i < nunits; i++) {
	    for (j = 0; j < num_weights_to[i]; j++) {
		if (epsilon[i][j] != 0.0)
		    epsilon[i][j] = lrate;
	    }
	}
    }
    if (bepsilon != NULL) {
	for (i = 0; i < nunits; i++) {
	    if (bepsilon[i] != 0.0)
		bepsilon[i] = lrate;
	}
    }
}

/* given a defined system, we will write the matrix and the biases 
   out to a file.  The file format is one floating point number per line,
   with the weight matrix in row major format followed by the biases.
*/

write_weights() {
    int     i,j,end;
    char   *str = NULL;
    char fname[BUFSIZ];
    char *star_ptr;
    char tstr[40];
    FILE * iop;

    if (weight == NULL) {
	return(put_error("cannot save undefined network"));
    }

nameagain:
    str = get_command("weight file name: ");
    if (str == NULL) return(CONTINUE);
    strcpy(fname,str);
    if ( (star_ptr = index(fname,'*')) != NULL) {
    	strcpy(tstr,star_ptr+1);
    	sprintf(star_ptr,"%d",epochno);
	strcat(fname,tstr);
    }
    if ((iop = fopen(fname, "r")) != NULL) {
    	fclose(iop);
        get_command("file exists -- clobber? ");
	if (str == NULL || str[0] != 'y') {
	   goto nameagain;
	}
    }
    if ((iop = fopen(fname, "w")) == NULL) {
	return(put_error("cannot open file for output"));
    }

    for (i = 0; i < nunits; i++) {
       for (j = 0; j < num_weights_to[i]; j++) {
	fprintf(iop, "%f\n", weight[i][j]);
       }
    }

    if (bias) {
      for (i = 0; i < nunits; i++) {
	 fprintf(iop, "%f\n", bias[i]);
      }
    }

    if (sigma) {
      for (i = 0; i < nunits; i++) {
	 fprintf(iop, "%f\n", sigma[i]);
      }
    }

    (void) fclose(iop);
    return(CONTINUE);
}


read_weights() {
    int     i,j,end;
    char   *str = NULL;
    FILE * iop;

    if(!System_Defined)
       if(!define_system())
	  return(BREAK);

    if (weight == NULL) {
	return(put_error("cannot restore undefined network"));
    }

    if((str = get_command("File name for stored weights: ")) == NULL)
	return(CONTINUE);

    if ((iop = fopen(str, "r")) == NULL) {
    	sprintf(err_string,"Cannot open weight file %s.",str);
	return(put_error(err_string));
    }

    for (i = 0; i < nunits; i++) {
	if(num_weights_to[i] == 0) continue;
	for (j = 0; j < num_weights_to[i]; j++) {
	  if (fscanf(iop, "%f", &weight[i][j]) == 0) {
	  fclose(iop);
	    return(put_error("weight file is not correct for this network"));
	  }
	}
    }

    end = nunits;
    
    if (bias != NULL) {
      for (i = 0; i < end; i++) {
	if (fscanf(iop, "%f", &bias[i]) == 0) {
	    fclose(iop);
	    return(put_error("weight file is not correct for this network"));
	}
      }
    }

    if (sigma != NULL) {
      for (i = 0; i < end; i++) {
	if (fscanf(iop, "%f", &sigma[i]) == 0) {
	    fclose(iop);
	    return(put_error("weight file is not correct for this network"));
	}
      }
    }

    (void) fclose(iop);
    update_display();
    return(CONTINUE);
}

/* realloc positive_constraints, negative_constraints, and link constraints
   this is called whenever the allocated constraint lists run out of
   space for additional constraints  14-May-87 MAF / 15-May-87 JLM */

#define CON_INCR 100 /* increment in size of constriant */

enlarge_constraints(con_index) int con_index; {
    int j;
    
    if (con_index == ENLARGE_POS) {
	maxpos += CON_INCR;
	positive_constraints = ((float **) erealloc 
	  ((char *) positive_constraints,
	      (unsigned int) ((maxpos - CON_INCR) * sizeof(float *)),
	      (unsigned int) (maxpos * sizeof(float *))));
	for (j = maxpos - CON_INCR; j < maxpos; j++) {
		positive_constraints[j] = NULL;
	}
    }
    else if (con_index == ENLARGE_NEG) {
	maxneg += CON_INCR;
	negative_constraints = ((float **) erealloc 
	  ((char *) negative_constraints,
	     (unsigned int) ((maxneg -CON_INCR) * sizeof (float *)),
	     (unsigned int) (maxneg * sizeof(float *))));
	for (j = maxneg - CON_INCR; j < maxneg; j++) {
		negative_constraints[j] = NULL;
	}
    }
    else {
	constraints[con_index].max += CON_INCR;
        constraints[con_index].cvec = ((float **) erealloc 
	  ((char *)constraints[con_index].cvec,
	   (unsigned int)
	     ((constraints[con_index].max - CON_INCR) * sizeof(float *)),
	   (unsigned int)
	     (constraints[con_index].max * sizeof(float *))));
        constraints[con_index].ivec = ((float **) erealloc 
	  ((char *)constraints[con_index].ivec,
	   (unsigned int)
	     ((constraints[con_index].max - CON_INCR) * sizeof(float *)),
	   (unsigned int)
	     (constraints[con_index].max * sizeof(float *))));
	for (j = constraints[con_index].max - CON_INCR; 
		j < constraints[con_index].max; j++) {
	    constraints[con_index].cvec[j] = NULL;
	    constraints[con_index].ivec[j] = NULL;
	}
   }
}
