/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* file: ia.h

	Header file for the ia program.
	
	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

#define AlphaMenu -255
#define BetaMenu  -254
#define GammaMenu -253
#define ThreshMenu -252
#define MaxMenu -251
#define MinMenu -250
#define RestMenu -249
/* #define ImaxMenu 248 */
#define ProbMenu -247
#define EstrMenu -246
#define ListsMenu -245
#define OscaleMenu -244

#define NUM_VALS	7
#define E 	2.718281828
#define NFIELDS	7
#define NWORD	1179
#define WLEN	4
#define NLET	26
#define	LLEN	14
#define	NFET	2
#define F	0
#define FU	1
#define	LD	2
#define L	3
#define LU	4
#define WD	5
#define W	6
#define NOWIN	-1

#define WA	0
#define	LA	1
#define WO	2
#define	LO	3

extern char word[NWORD][5];
extern float freq[NWORD];
extern int uc[26][14];

extern int quiet;
extern float a[],b[],g[],t[],max[],min[],rest[],estr[],fdprob[],
       oscale[],dthresh[];
       
extern float wa[NWORD];
extern float l[WLEN][NLET];
extern float outrate,fgain;
extern int ncycles;
extern int compute_resprob;
extern int cycleno,tally;
extern int tal[];
extern float prsum,prsm[];
extern int fc_pos,fc_let[2];
extern float fc_max;
extern char *trial_history[NFIELDS];
extern char th[NFIELDS][10];
extern char    *disp_word_ptr[30];
extern char    *disp_fc_ptr[2];
extern char	*disp_let_ptr[WLEN][15];
extern float	disp_word_act[30];
extern float	disp_fc_act[2];
extern float   disp_let_act[WLEN][15];
extern float	disp_word_rpr[30];
extern float	disp_fc_rpr[2];
extern float   disp_let_rpr[WLEN][15];
