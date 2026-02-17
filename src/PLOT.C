/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* file: plot.c

    Simple program that makes x[i] vs y plots for 23x79 display
    
    First version implemented by JLM
    
    Date of last revision:  8-12-87/JLM
    
*/

#include <stdio.h>
#include <math.h>
#include <ctype.h>

#define C_MIN		9
#define C_MAX		69
#define L_MIN		0
#define L_MAX		20
#define C_RANGE		(C_MAX - C_MIN)
#define L_RANGE		(L_MAX - L_MIN)
#define C_CENTER	(C_MIN + C_RANGE/2)
#define L_CENTER	(L_MIN + L_RANGE/2)

struct dimension {
	double min;
	double max;
	double val;
	double scale;
	int l;
	char title[20];
} x, y;

char in[100];
char line[BUFSIZ];
char graph_title[20];
char label[26][20];
FILE *format, *data, *plot;

int nlabels = 0;
char symbol = '\0';
char screen[23][80];

char *
next(sp,lp) char *sp,*lp; {
	while(*lp && isspace(*lp)) *lp++;
	if (*lp == '\0') return(NULL);
	while(*lp && !isspace(*lp)) *sp++ = *lp++;
	*sp = '\0';
	return(lp);
}

main(argc,argv) int argc; char **argv; {
	if ( (format = fopen(argv[1],"r")) == NULL) {
		fprintf(stderr,"Can't open format file.");
		exit(1);
	}
	if ( (data = fopen(argv[2],"r")) == NULL) {
		fprintf(stderr,"Can't open data file.");
		exit(1);
	}
	if (argc > 3) {
	    if ( (plot = fopen(argv[3],"w")) == NULL) {
		fprintf(stderr,"Can't open output file.");
		exit(1);
	    }
	}
	else plot = stdout;
	get_format();
	make_plot();
	put_plot();
}

get_format() {
	int label_index = 0;
	x.min = 0.0;
	x.max = 50.0;
	y.min = 0.0;
	y.max = 1.0;
	while (fscanf(format,"%s",in) != EOF) {
		switch (in[0]) {
		  case 'x':
		  	get_dimspec(&x);
			break;
		  case 'y':
		  	get_dimspec(&y);
			break;
		  case 't':
		  	fscanf(format,"%s",graph_title);
			break;
		  case 'l':
			fscanf(format,"%s",label[label_index++]);
			break;
		  case 's':
		  	fscanf(format,"%s",in);
			symbol = in[0];
			break;
		  default:
		  	fprintf(stderr,"bad format specification.\n");
			exit(1);
		}
	}
	nlabels = label_index;
	if (x.l) {x.max = log10(x.max); x.min = log10(x.min);}
	if (y.l) {y.max = log10(y.max); y.min = log10(y.min);}
	x.scale = x.max - x.min;
	y.scale = y.max - y.min;
}

get_dimspec(dim) struct dimension *dim; {
	fscanf(format,"%s",in);
	switch(in[0]) {
		case 'M':
			fscanf(format,"%lf",&dim->max);
			break;
		case 'm':
			fscanf(format,"%lf",&dim->min);
			break;
		case 't':
			fscanf(format,"%s",dim->title);
			break;
		case 'l':
			dim->l = 1;
			break;
		default:
			return;
	}
}

make_plot() {
	label_plot();
	insert_data();
}

label_plot() {
	int l,c,len,i,j;
	
	for (c = C_MIN-1; c <= C_MAX+1; c++) {
		screen[L_MAX+1][c] = screen[L_MIN][c] = '-';
	}
	for (l = L_MIN; l <= L_MAX; l++) {
		screen[l][C_MIN-1] = screen[l][C_MAX+1] = '|';
	}
	len = strlen(graph_title);
	strcpy(&screen[L_MIN][C_CENTER-(len/2)],graph_title);
	screen[L_MIN][C_CENTER-(len/2)+len] = '-';
	len = strlen(x.title);
	strcpy(&screen[L_MAX+1][C_CENTER-(len/2)],x.title);
	screen[L_MAX+1][C_CENTER-(len/2)+len] = '-';
	len = strlen(y.title);
	for (i = 0, l = L_CENTER - (len/2);i < len; i++,l++) {
		screen[l][0] = y.title[i];
	}
	sprintf(&screen[L_MIN][2],"%5.2f",y.max);
	sprintf(&screen[L_CENTER][2],"%5.2f",(y.min + (y.max - y.min)/2.0));
	sprintf(&screen[L_MAX][2],"%5.2f",y.min);
	sprintf(&screen[L_MAX+2][C_MIN-4],"%7.2f",x.min);
	sprintf(&screen[L_MAX+2][C_CENTER-4],"%7.2f",
					     (x.min + (x.max - x.min)/2.0));
	sprintf(&screen[L_MAX+2][C_MAX-4],"%7.2f",x.max);
	for (i = 0; i < nlabels; i++) {
		sprintf(&screen[i+2][C_MAX+3],"%c", (char) i + 'a');
		len = strlen(label[i]);
		for (c = C_MAX+5,j = 0; j < len && c < 79; c++,j++) {
			screen[i+2][c]= label[i][j];
		}
	}
}

insert_data() {
	int i,l,c;
	char *sp;
	char *lp;
	double tx,ty;
	while(fgets(line,BUFSIZ,data) != NULL) {
	    if ((lp = next(in,line)) == NULL) continue;
	    sscanf(in,"%lf",&tx);
	    if (x.l) x.val = log10(tx);
	    else x.val = tx;
	    c = C_MIN + ((double)(C_RANGE)/x.scale)*(x.val-x.min);
	    if (c < C_MIN || c > C_MAX) continue;
	    for (i = 0; (lp = next(in,lp)) != NULL; i++) {
		if (sscanf(in,"%lf",&ty) == 0) continue;
		if (y.l) y.val = log10(ty);
		else y.val = ty;
		l = L_MAX - (int)(L_RANGE*((y.val - y.min)/y.scale));
		if (l <= L_MIN || l > L_MAX) continue;
		sp = &screen[l][c];
		if (symbol) *sp = symbol;
		else if (*sp == '\0') *sp = i + 'a';
		else if (*sp >= 'a' && *sp <= 'z') *sp = '2';
		else if (*sp >= '2' && *sp <= '8') *sp += 1;
		else if (*sp == '9') *sp = '*';
	    }
	}
}

put_plot() {
	int l,c;
	char ch;
	for (l = 0; l < 23; l++) {
	     for (c = 0; c < 79; c++) {
	     	ch = screen[l][c];
		if (ch == '\0') putc(' ',plot);
		else putc(ch,plot);
	     }
	     putc('\n',plot);
	}
}
	   
	
