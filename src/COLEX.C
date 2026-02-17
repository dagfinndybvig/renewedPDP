/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* file: colex.c

    Simple program that extracts columns from files consisting of
    lines each with many columns.  
    
    First version implemented by JLM - 5-16-87
    
    Date of last revision 8-12-87.

*/    


#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
FILE *in, *out, *abut;
int outcol[10];
char input_line[BUFSIZ];
char outstr[BUFSIZ];


nextcol(count) int count; {
	int i;
	char *lp = input_line;
	char *sp = outstr;
	
	while(*lp && isspace(*lp)) *lp++;
	for (i = 0; i < count; i++) {
		while(*lp && !isspace(*lp)) *lp++;
		while(*lp &&  isspace(*lp)) *lp++;
	}
	if (*lp == '\0') return (0);
	while(*lp && !isspace(*lp)) *sp++ = *lp++;
	*sp = '\0';
	return(1);
}
	
main(argc,argv) int argc; char **argv; {
	int arg;
	char *lp;
	int oc,noc,len,t;

	arg = 3;

	if ( (in = fopen(argv[1],"r")) == NULL) {
		fprintf(stderr,"Cannot open input file.\n");
		exit(1);
	}
	if ( (out = fopen(argv[2],"w")) == NULL) {
		fprintf(stderr,"Cannot open output file.\n");
		exit(1);
	}
	if (argc > 3 && sscanf(argv[3],"%d",&t) ==0) {
	     if ( (abut = fopen(argv[arg++],"r")) == NULL) {
		fprintf(stderr,"Cannot open file to abut to.\n");
		exit(1);
	     }
	}

	for (oc = 0; arg < argc && oc < 10; arg++,oc++) {
		outcol[oc] = atoi(argv[arg]);
	}
	noc = oc;
	if (abut) {
	    while (fgets(input_line,BUFSIZ,abut) != NULL) {
		len = strlen(input_line);
		input_line[len-1] = '\0';
		fprintf(out,"%s ",input_line);
		if (fgets(input_line,BUFSIZ,in) != NULL) {
		  for (oc = 0; oc < noc; oc++) {
		   if (nextcol(outcol[oc]) == NULL) {
			fprintf(stderr,"Not enough columns.\n");
			exit(1);
		   }
		   fprintf(out,"%s ",outstr);
		  }
		}
	        else {
		  for(oc = 0; oc < noc; oc++) {
		   fprintf(out,"* ");
		  }
		}
		fprintf(out,"\n");
	    }
	}
	else {
	    while (fgets(input_line,BUFSIZ,in) != NULL) {
		for (oc = 0; oc < noc; oc++) {
		   if (nextcol(outcol[oc]) == NULL) {
			fprintf(stderr,"Not enough columns.\n");
			exit(1);
		   }
		   fprintf(out,"%s ",outstr);
		}
		fprintf(out,"\n");
	    }
	}
}
