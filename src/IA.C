/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/*

    Guts if interactive activation model.

    Initial adaptation by Eliot Jaffe, 8-86 from the original
    program of Rumelhart and McClelland.
    
    Date of last revision:  8-12-87/JLM.

*/        

#include "ia.h"
#include "io.h"
#include <math.h>
#include "general.h"

int nunits = NWORD+WLEN*(NLET+NFET*LLEN);
int ninputs, noutputs;
int System_Defined = 1;

char alphabet[26][2] = {"a","b","c","d","e","f","g","h","i","j",
		        "k","l","m","n","o","p","q","r","s","t",
		        "u","v","w","x","y","z"};

char uc_alphabet[26][2] = {"A","B","C","D","E","F","G","H","I","J",
		        "K","L","M","N","O","P","Q","R","S","T",
		        "U","V","W","X","Y","Z"};

float   a[7] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
float   g[7] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
/* the default values are: */
/* float   a[7] = { 0.0, 0.005, 0.0, 0.0, 0.07, 0.30, 0.0}; */
/* float   g[7] = { 0,.15, 0, 0,.04, 0, .21 }; */
/* for the sake of the exercises, we've put these in ia.par */
float oscale[7] = { 0.0, 0.0, 0.0, 10.0, 0.0, 0.0, 20.0};
float   min[7] = { 0, 0, 0, -.2, 0, 0, -.2 };
float   b[7] = { 0, 0, 0, .07, 0, 0, .07 };
float   t[7] = { 0, 0, 0, 0, 0, 0, 0};
float   rest[7] = { 0, 0, 0, 0, 0, 0, 0};
float   max[7] =  { 0, 0, 0, 1, 0, 0, 1};
float	dthresh[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

int     ftime[NFIELDS];
float   fdprob[NFIELDS] = {   1, 1, 1, 1, 1, 1, 1};
float   estr[WLEN] = { 1.0, 1.0, 1.0, 1.0};
float   outrate = .05;
float   fgain = .05;

char	*trial_history[NFIELDS];
char	th[NFIELDS][10];
char    field[NFIELDS][WLEN + 1];

char    fetbuf[NFET][WLEN][LLEN + 1] = {
    "00000000000000", "00000000000000",
    "00000000000000", "00000000000000",
    "00000000000000", "00000000000000",
    "00000000000000", "00000000000000"
};

float   input[NFET][LLEN][WLEN];
float   dinput[NFET][LLEN][WLEN];
double  out[WLEN][NLET];
double  wout[NWORD];
float   blankin = .2;
int     cycleno = 0;
int     fieldno = 0;
int     ncycles = 10;
int	compute_resprob = 1;
float   prsum, sum, sm[WLEN], prsm[WLEN];

int     tally, tal[WLEN];
float   wa[NWORD],
        wr[NWORD];
float   ew[NWORD],
        iw[NWORD];
float  *wp,*wrp;
float  *ewp,
       *iwp;
double *owp;
float   l[WLEN][NLET],
        el[WLEN][NLET],
        il[WLEN][NLET];
float   ss,
        ssm[WLEN];
int     numw = 0;
int     numl[WLEN] = {0,0,0,0};
int     wi[30];
int li[WLEN][15];
int	fc_defined;
int	list_defined;
int	fc_pos;
int	fc_let[2];
float	fc_max;

char    *disp_word_ptr[30];
char    *disp_fc_ptr[2] = {"-", "-"};
char	*disp_let_ptr[WLEN][15];
float	disp_word_act[30];
float	disp_fc_act[2];
float   disp_let_act[WLEN][15];
float	disp_word_rpr[30];
float	disp_fc_rpr[2];
float   disp_let_rpr[WLEN][15];
int 	cnw, cnl[WLEN];


setinput() {
    register int i,j,k;
    if (getlet(field[fieldno]) == BREAK) return(BREAK);
    for (k = 0; k < WLEN; k++) {
	for (j = 0; j < LLEN; j++) {
	    for (i = 0; i < NFET; i++) {
		dinput[i][j][k] = 0;
	    }
	    if (rnd() < fdprob[fieldno]) {
		for (i = 0; i < NFET; i++) {
		    dinput[i][j][k] =
			input[i][j][k];
		}
	    }
	}
    }
    fieldno++;
    return(CONTINUE);
}

cycle() {
    register int    i,
                    j,
                    k;
    int     tme;
    for (tme = 0; tme < ncycles; tme++) {
	cycleno++;
	if (cycleno == ftime[fieldno]) {
	    if (setinput() == BREAK) {
	    	update_out_values();
	    	update_display();
	    	return(BREAK);
	    }
	}
	interact();
	wupdate();
	lupdate();
	if (step_size == CYCLE) {
	    update_out_values();
	    update_display();
	    if (single_flag) {
		if (contin_test() == BREAK) return(BREAK);
	    }
	}
	if (Interrupt) {
    	   Interrupt_flag = 0;
	   update_display();
	   if (contin_test() == BREAK) return(BREAK);
	}
    }
    if (step_size > CYCLE) {
	update_out_values();
	update_display();
    }
    return(CONTINUE);
}

interact() {
    register int    i, j, k, m;
    float out, eout, iout;
    
/* letter -> word */
    for (i = 0; i < NLET; i++) {
	for (j = 0; j < WLEN; j++) {
	    out = l[j][i] - t[L];
	    if (out > 0) {
	    	eout = a[LU] * out;
		iout = g[LU] * out;
		for (k = 0; k < NWORD; k++) {
		    if (i == (word[k][j] - 'a'))
			ew[k] += eout;
		    else
			iw[k] += iout;
		}
	    }
	}
    }
    
/* word -> letter */
    for (k = 0; k < NWORD; k++) {
	out = wa[k] - t[WD];
	if (out > 0) {
	    eout = a[WD] * out;
            for (j = 0; j < WLEN; j++) {
		i = (word[k][j] - 'a');
		el[j][i] += eout;
	    }
	}
    }
    
/* feature -> letter */
    for (j = 0; j < WLEN; j++) {
        for (k = 0; k < LLEN; k++) {
	    for (m = 0; m < NFET; m++) {
		out = dinput[m][k][j];
		if (out > 0) {
		    eout = estr[j] * a[FU] * out;
		    iout = estr[j] * g[FU] * out;
		    for (i = 0; i < NLET; i++) {
			if (m == uc[i][k])
			    el[j][i] += eout;
			else
			    il[j][i] += iout;
		    }
		}
	    }
	}
    }
}


wupdate() {
    int     i;
    float net, effect;
    ss = sum;
    prsum = sum = 0;
    tally = 0;

    /* pointers for efficiency */
    for (wp = wa, wrp = wr,  ewp = ew, iwp = iw, owp = wout;
	 wp < wa + NWORD; 
	 wp++,wrp++,ewp++,iwp++,owp++) {
	if (*wp > t[W])
	    *iwp += g[W] * (ss - (*wp - t[W]));
	else
	    *iwp += g[W] * ss;
	net = *ewp - *iwp;
	if (net > 0)
	    effect = (max[W] - *wp) * (net);
	else
	    effect = (*wp - min[W]) * (net);
	*wp += effect - b[W]*(*wp - *wrp);
	if (*wp > 0) {
	    if (*wp > max[W]) *wp = max[W];
	    tally++;
	    prsum += *wp;
	}
	else {
	    if (*wp < min[W]) *wp = min[W];
	}
	if (*wp > t[W])	sum += *wp - t[W];
	if (compute_resprob == 2) {
	    *owp = *owp * (1 - outrate) + *wp * outrate;
	}
	*ewp = *iwp = 0;
    }
}

lupdate() {
    register int    i,j;
    float elv, ilv,lv;		    
    float net, effect;

    for (i = 0; i < WLEN; i++) {
	ssm[i] = sm[i];
	tal[i] = prsm[i] = sm[i] = 0;
    }

    for (j = 0; j < WLEN; j++) {
	for (i = 0; i < NLET; i++) {
	    lv = l[j][i];
	    elv = el[j][i];
	    ilv = il[j][i];
	    if (lv > t[L])
		ilv += g[L] * (ssm[j] - (lv - t[L]));
	    else
		ilv += g[L] * ssm[j];
	    net = elv - ilv;
	    if (net > 0)
		effect = (max[L] - lv) * net;
	    else
		effect = (lv - min[L]) * net;
	    lv += effect - b[L]*(lv - rest[L]);
	    if (lv > 0) {
		if (lv > 1)
		    lv = 1;
		tal[j]++;
		if (lv > t[L])
		    sm[j] += lv - t[L];
		prsm[j] += lv;
	    }
	    else
		if (lv < min[L])
		    lv = min[L];
	    if (compute_resprob > 0) {
		out[j][i] = out[j][i] * (1 - outrate) + lv * outrate;
	    }
	    l[j][i] = lv;
	    el[j][i] = il[j][i] = 0;
	}
    }
}

putfet(pos) int pos; {
    int     i;
    char    *tinbf;

    tinbf = get_command("absent: ");
    if (tinbf != NULL) {
	(void)strcpy(fetbuf[0][pos], tinbf);
    }
    else {
	(void)strcpy(fetbuf[0][pos], "00000000000000");
    }
    tinbf = get_command("present: ");
    if (tinbf != NULL) {
	(void)strcpy(fetbuf[1][pos], tinbf);
    }
    else {
	(void)strcpy(fetbuf[1][pos], "00000000000000");
    }
}

getfet(pos) {
    int i;
    for (i = 0; i < LLEN; i++) {
	input[0][i][pos] = fetbuf[0][pos][i] - '0';
    }
    for (i = 0; i < LLEN; i++) {
	input[1][i][pos] = fetbuf[1][pos][i] - '0';
    }
}

mkkr(pos)
int     pos;
{
    int     i;
    char tstring[40];

    (void)strcpy(tstring, "00001100011001");
    for (i = 0; i < LLEN; i++) {
	input[0][i][pos] = (float) (tstring[i] - '0');
    }
    (void)strcpy(tstring, "11000010000010");
    for (i = 0; i < LLEN; i++) {
	input[1][i][pos] = (float) (tstring[i] - '0');
    }
}

getlet(in)
char   *in;
{
    int     i,
            j,
            k,
            index;

    for (k = 0; k < WLEN; k++) {
	if (in[k] == '_') {
	    for (i = 0; i < LLEN; i++)
		input[0][i][k] = input[1][i][k] = 0;
	    continue;
	}
	if (in[k] == '"') {
	    getfet(k);
	    continue;
	}
	if (in[k] == '*') {
	    mkkr(k);
	    continue;
	}
	if (in[k] == '#') {
	/* the X-O mask character */
	    for (i = 0; i < 6; i++) {
		input[0][i][k] = 0.0;
		input[1][i][k] = 1.0;
	    }
	    for (; i < 10; i++) {
		input[0][i][k] = 1.0;
		input[1][i][k] = 0.0;
	    }
	    for (; i < LLEN; i++) {
		input[0][i][k] = 0.0;
		input[1][i][k] = 1.0;
	    }
	    continue;
	}
	if (in[k] == '.') {
	    for (i = 0; i < LLEN; i++) {
		input[0][i][k] = blankin;
		input[1][i][k] = 0;
	    }
	    continue;
	}
	if (in[k] == '?') {
	    for (i = 0; i < LLEN; i++) {
		if (rnd() >=.5)
		    j = 1;
		else
		    j = 0;
		input[1 - j][i][k] = 0;
		input[j][i][k] = 1.0;
	    }
	    continue;
	}
	if (isupper(in[k])) {
	  index = in[k] - 'A';
	}
	else {
	  put_error("Invalid character encountered in trial specification.");
	  return(BREAK);
	}
	for (i = 0; i < LLEN; i++) {
	    j = uc[index][i];
	    input[1 - j][i][k] = 0;
	    input[j][i][k] = 1.0;
	}
    }
    return(CONTINUE);
}

zarrays() {
    int     i,
            j,
            k;
    srand(random_seed);
    cycleno = 0;
    tally = 0;
    fieldno = 0;
    prsum = sum = ss = 0;
    for (i = 0; i < NWORD; i++) {
	ew[i] = iw[i] = 0;
	wout[i] = wa[i] = wr[i] = fgain * freq[i] + rest[W];
    }
    for (j = 0; j < WLEN; j++)
	for (k = 0; k < NLET; k++)
	    l[j][k] = out[j][k] = 0 + rest[L];
    el[j][k] = il[j][k] = 0;
    for (i = 0; i < WLEN; i++) {
	for (j = 0; j < LLEN; j++) {
	    for (k = 0; k < NFET; k++)
		dinput[k][j][i] = input[k][j][i] = 0;
	}
    }
    for (i = 0; i < WLEN; i++) {
	prsm[i] = sm[i] = ssm[i] = 0;
	tal[i] = 0;
    }
    fc_max = 0.0;
    update_out_values();
    return(CONTINUE);
}

trial() {
    int     i,j,pos;
    int	    prevtime;
    char    *str;
    char    tempstr[40];
    char    ststr[40];

    prevtime = 0;
    
    for (i = 0; i < NFIELDS; i++) {
	trial_history[i] = &th[i][0];
    }
    for (i = 0; i < NFIELDS; i++) {
gettime:    
	(void)sprintf(tempstr,"field #%d: time: ",i);
	str = get_command(tempstr);
	if (!str || (strcmp(str,"end") == 0)) {
	    ftime[i] = 0;
	    break;
	}
	if (sscanf(str,"%d",&j) == 0) {
		put_error("Time must be an integer.");
		goto gettime;
	}
	if (j <= prevtime) {
		put_error("Times must be strictly increasing.");
		goto gettime;
	}		
	ftime[i] = j;
	strcpy(ststr,tempstr);
	(void) sprintf(tempstr,"%s %d contents: ",ststr,j);
	str = NULL;
getstring:	
	str = get_command(tempstr);
	if (strlen(str) != 4) {
		put_error("Field contents must be 4 characters.");
		goto getstring;
	}
	for (pos = 0; pos < 4; pos++) {
	  if(islower(str[pos])) str[pos] = toupper(str[pos]);
	}
	(void)strcpy(field[i],str);
	for (pos = 0; pos < 4; pos++) {
	  if (field[i][pos] == '"') {
	    putfet(pos);
	  }
	}
        (void)sprintf(th[i],"%2d %s",ftime[i],field[i]);
    }
    change_variable_length("trial",i,10);
    newstart();
    return(CONTINUE);
}

clear_disp_list() {
    int i;
    for ( i = 0; i < WLEN; i++) {
        cnl[i] = numl[i] = 0;
    }
    cnw = numw = 0;
    list_defined = 0;
    change_lengths();
}

get_disp_list() {
    int     i;
    char   *in = NULL;
    char    temp[100];
    char    str[5];
    
    clear_disp_list();
    cnw = numw = 0;
    in = get_command("enter words or - for dynamic specification: ");
    if (in && in[0] == '-') return(CONTINUE);
    while (in != NULL && strcmp(in,"end") != 0) {
    	for (i = 0; i < WLEN; i++) {
	    if (in[i] && isupper(in[i])) {
	    	in[i] = tolower(in[i]);
	    }
	}
	for (i = 0; i < NWORD; i++) {
	    if (!strcmp(in, word[i])) {
		wi[numw++] = i;
		break;
	    }
	}
	if (i == NWORD) {
		sprintf(err_string,"Unrecognized word: %s.",in);
		put_error(err_string);
	}
	in = get_command("next word (end with end or <cr>): ");
    }
    cnw = numw;

    for (i = 0; i < WLEN; i++) {
        numl[i] = 0;
	sprintf(temp,"position %d, letter (end with end or <cr>): ", i);
	in = get_command(temp);
	while (in != NULL && strcmp(in,"end") != 0) {
	  if (isupper(in[0])) in[0] = tolower(in[0]);
	  if (islower(in[0])) {
	    li[i][numl[i]] = in[0] - 'a';
	  }
	  else {
	    put_error("Entries must be letters.");
	    goto tryagain;
	  }
	  numl[i]++;
tryagain:
	  in = get_command(temp);	  
	}
	cnl[i] = numl[i];
    }
    if (numw > 0 || numl[0] > 0 || numl[1] > 0 || numl[2] > 0 || numl[3] > 0){
      list_defined = 1;
      change_lengths();
    }
    update_out_values();
    return(CONTINUE);
}

fc() {
    int     i;
    char   *in = NULL;
    char    temp[40];
    char    str[5];
    
    fc_defined = 0;
    fc_max = 0;
getpos:    
    in = get_command("fc position (0 to 3, - to clear): ");
    if (in == NULL || in[0] == '-') {
        fc_pos = 0;
	disp_fc_ptr[0] = "-";
	disp_fc_ptr[1] = "-";
	fc_let[0] = fc_let[1] = 0;
	fc_max = 0.0;
	disp_fc_act[0] = disp_fc_act[1] = 0.0;
	disp_fc_rpr[0] = disp_fc_rpr[1] = 0.0;
	return(CONTINUE);
    }
    if (sscanf(in,"%d",&fc_pos) != 1) {
		put_error("Must give an position number!");
		goto getpos;
    }
    if (fc_pos < 0 || fc_pos > 3) {
    		put_error("Position must be 0, 1, 2, or 3.");
		goto getpos;
    }
getcor:    
    in = get_command("correct alternative: ");
    if (!in) {
	put_error("Must specify a letter.");
	goto getcor;
    }
    if (islower(in[0])) {
	in[0] = toupper(in[0]);
    }
    if (isupper(in[0])) fc_let[0] = in[0] - 'A';
    else {
    	put_error("Alternatives must be letters.");
	goto getcor;
    }
getinc:    
    in = get_command("incorrect alternative: ");
    if (!in) {
	put_error("Must specify a letter.");
	goto getinc;
    }
    if (islower(in[0])) {
       in[0] = toupper(in[0]);
    }
    if (isupper(in[0])) fc_let[1] = in[0] - 'A';
    else {
    	put_error("Alternatives must be letters.");
	goto getinc;
    }
    disp_fc_ptr[0] = uc_alphabet[fc_let[0]];
    disp_fc_ptr[1] = uc_alphabet[fc_let[1]];
    fc_defined = 1;
    update_display();
    return(CONTINUE);
}

printout() {
    int     i,
            j,
            cnt,
            lcnt;
    char   *str;
    double  lpow;
    double  tstr;

    lpow = exp(oscale[L]);
    
    str = NULL;
    while(str == NULL)
      str = get_command("print words? ");
    cnt = 0;
    lcnt = 0;
    if (*str == 'y') {
	io_move(1,0);
	clear_display();
	for (i = 0; i < NWORD; i++) {
		io_printw("%s ", word[i]);
		io_printw("%.2f ", wa[i]);
		cnt++;
		if (cnt % 7 == 0) {
		    lcnt++;
		    io_printw("\n",NULL);
		    if (lcnt % 23 == 0) {
			if (contin_test() == BREAK) {
			  break;
			}
			else {
			    io_move(1,0);
			    clear_display();
			}			  
		    }
		}
	}
    }
    str = NULL;
    while(str == NULL)
	  str = get_command("print letters? ");
    if (*str == 'y') {
	clear_display();
	io_move(1,0);
	for (i = 0; i < NLET / 2; i++) {
	    io_printw("%c ", i + 'a');
	    for (j = 0; j < WLEN; j++)
		    io_printw("%6.3f ", l[j][i]);
	    io_printw("	",NULL);
	    io_printw("%c ", i + 13 + 'a');
	    for (j = 0; j < WLEN; j++)
		    io_printw("%6.3f ", l[j][i + 13]);
	    io_printw("\n",NULL);
	}
    }
    str = NULL;
    while(str == NULL)
       str = get_command("print letter resp-probs? ");
  if (str[0] == 'y') {
    clear_display();
    io_move(1,0);
    for (i = 0; i < WLEN; i++) {
	ssm[i] = 0;
	for (j = 0; j < NLET; j++)
	    ssm[i] += pow(lpow, out[i][j]);
    }
    for (i = 0; i < NLET / 2; i++) {
	io_printw("%c ", i + 'a');
	for (j = 0; j < WLEN; j++) {
	    tstr = pow(lpow, out[j][i]) / ssm[j];
	    io_printw("%.3f ", tstr );
	}
	io_printw("	",NULL);
	io_printw("%c ", i + 13 + 'a');
	for (j = 0; j < WLEN; j++) {
	    tstr = pow(lpow, out[j][i + 13]) / ssm[j];
	    io_printw("%.3f ", tstr );
	}
	io_printw("\n",NULL);
    }
    str = get_command("enter <cr> to return to top level: ");
  }
  clear_display();
  update_display();
  return(CONTINUE);
}

update_out_values() {
    int     i,pos,j,k;
    double denom;
    double c0,c1;
    double wpow;
    double lpow;
    lpow = exp(oscale[L]);
    wpow = exp(oscale[W]);
    
    if (list_defined == 0) {
    	make_disp_lists();
    }

    for (i = 0; i < numw; i++) {
        disp_word_ptr[i] = word[wi[i]];
	disp_word_act[i] = wa[wi[i]];
    }
    if (compute_resprob > 1) {
      if (numw > 0) {
	denom = 0;
	for (i = 0; i < NWORD; i++) {
	    denom += pow(wpow, wout[i]);
	}
      }
      for (i = 0; i < numw; i++) {
	disp_word_rpr[i] = pow(wpow,wout[wi[i]])/denom;
      }
    }
    for (pos = 0; pos < WLEN; pos++) {
      for (i = 0; i < numl[pos]; i++) {
        disp_let_ptr[pos][i] = alphabet[li[pos][i]];
	disp_let_act[pos][i] = l[pos][li[pos][i]];
      }
      if (compute_resprob > 0) {
        if (numl[pos] > 0) {
	    denom = 0;
	    for (i = 0; i < NLET; i++) {
		denom += pow(lpow, out[pos][i]);
	    }
	}
        for (i = 0; i < numl[pos]; i++) {
	  disp_let_rpr[pos][i] = 
	        pow(lpow, out[pos][li[pos][i]])/denom;
	}
      }
    }
    if (fc_defined) {
     disp_fc_act[0] = l[fc_pos][fc_let[0]];
     disp_fc_act[1] = l[fc_pos][fc_let[1]];
     if (compute_resprob > 0) {
       c0 = pow(lpow,out[fc_pos][fc_let[0]]);
       c1 = pow(lpow,out[fc_pos][fc_let[1]]);
       denom = 0;
       for (i = 0; i < NLET; i++) {
		denom += pow(lpow, out[fc_pos][i]);
       }
       c0 /= denom;
       c1 /= denom;
       disp_fc_rpr[0] = c0 + .5*(1.0 - (c0 + c1));
       disp_fc_rpr[1] = 1.0 - disp_fc_rpr[0];
       if (disp_fc_rpr[0] > fc_max) fc_max = disp_fc_rpr[0];
     }
    }
}

make_disp_lists() {
	register int i,pos;
	int onw,onl[WLEN],uf;
	
	onw = numw;
	numw = 0;
	for (i = 0; i < NWORD; i++) {
	   if (wa[i] > dthresh[WA]) wi[numw++] = i;
	   if (numw == 30) break;
	}
	for (pos = 0; pos < WLEN; pos++) {
	   onl[pos] = numl[pos];
	   numl[pos] = 0;
	   for (i = 0; i < NLET; i++) {
	     if (l[pos][i] > dthresh[LA]) {
	         li[pos][numl[pos]] = i;
	         numl[pos]++;
	     }
	     if (numl[pos] == 15) break;
	   }
	}
	cnw = numw;
	if (onw > numw) {
	   for (i = numw; i < onw; i++) {
		disp_word_ptr[i] = "          ";
	   }
	   cnw = onw;
	}
	for (pos = 0; pos < WLEN; pos++) {
	    cnl[pos] = numl[pos];
	    if (onl[pos] > numl[pos]) {
	       for (i = numl[pos]; i < onl[pos]; i++) {
	        disp_let_ptr[pos][i] = "          ";
	       }
	       cnl[pos] = onl[pos];
	    }
	}
	change_lengths();
}

change_lengths() {
	change_variable_length("dwp",cnw,0);
	change_variable_length("dwa",numw,0);
	change_variable_length("dwr",numw,0);
	change_variable_length("dlp0",cnl[0],0);
	change_variable_length("dla0",numl[0],0);
	change_variable_length("dlr0",numl[0],0);
	change_variable_length("dlp1",cnl[1],0);
	change_variable_length("dla1",numl[1],0);
	change_variable_length("dlr1",numl[1],0);
	change_variable_length("dlp2",cnl[2],0);
	change_variable_length("dla2",numl[2],0);
	change_variable_length("dlr2",numl[2],0);
	change_variable_length("dlp3",cnl[3],0);
	change_variable_length("dla3",numl[3],0);
	change_variable_length("dlr3",numl[3],0);
}
	

define_system() {
	System_Defined = 1;
}

/*
    double  oscale[L] = 22026.4658,
    double  oscale[L] = 22026.4658;
    double  oscale[W] = 485165195.4;
*/
