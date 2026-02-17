/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* command.c

	the root command routines.
	
	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

/* 
	The command routines are mostly table driven.
*/

#include "general.h"
#include "io.h"
#include "command.h"
#include <string.h>
#include <stdlib.h>

int maxcommands = MAXCOMMANDS;
/* maxcommands is automatically increased if necessary */

static int  Current_Menu;
int endline = 1;
int error_flag = 0;
int file_err = 0;
int last_entry = 0;

/* This function is the top level of the command interface.
   it takes a string, that it will use as the prompt, and a table
   pointer, that will be used to parse the command.  The function prints
   the prompt.  Then, it gets a string. Matches the string on the names
   in the Command table, and if there is a match, it will call the
   appropiate function.  Specialized command routines are set up by
   puting there name in the command table and having a command call them.
*/

do_command(str, current_menu)
char   *str;
int     *current_menu;
{
    char   *get_command ();
    char   *strcat ();
    char   *command_string;
    char    new_command[128];
    int     matches, lastmatch, matchlen,tlen,i;
    
    Interrupt_flag = 0;

    Current_Menu = (int) current_menu;
    if (endline) {
    	do_help(str,Current_Menu);
    }

    command_string = get_command(str);

    if (command_string == (char *) NULL) {
    	if (Current_Menu == BASEMENU) return(POP);
	return(BREAK);
    }

    matches = 0;

    for (i = 0; i < last_entry; ++i) {
	if ((!Command[i].menutype) || (Command[i].menutype == Current_Menu)) {
	    if (!strcmp(Command[i].command, command_string)) {
		matches = 1;
		lastmatch = i;
		break;
	    }
	}
    }
    if (!matches) {		/* no exact matches, try partials */
	matchlen = strlen(command_string);
	for (i = 0; i < last_entry; ++i) {
	    if ((!Command[i].menutype) ||
		    (Command[i].menutype == Current_Menu)) {
		tlen = strlen(Command[i].command) -1;
		if (Command[i].command[tlen] == '/') {
			if (tlen > matchlen) tlen = matchlen;
		}
		else tlen = matchlen;
		if (!strncmp(Command[i].command, command_string, tlen)) {
		    ++matches;
		    lastmatch = i;
		}
	    }
	}
    }
    if (matches > 1) {
	if( Current_Menu == DISPLAYMENU) {
	    matchlen = strlen(command_string);
	    for (i = 0; i < last_entry; ++i) {
	        if ((Command[i].menutype == Current_Menu)) {
		    if (!strncmp(Command[i].command,command_string,matchlen)){
			    *new_command = '\0';
		      (void) strcat(new_command, str);
		      (void) strcat(new_command, " ");
		      (void) strcat(new_command,Command[lastmatch].command);
		      if ((*Command[i].func)(new_command,Command[i].arg) 
		      						== BREAK) {
			return(BREAK);
		      }
		    }
	        }
	    }
	}
	else {
	    sprintf(err_string,"ambiguous command: '%s'.  Choose one of: ",
							command_string);
	    for (i = 0; i < last_entry; ++i) {
	        if ((!Command[i].menutype) ||
		        (Command[i].menutype == Current_Menu)) {
		   if (!strncmp(Command[i].command,command_string,matchlen)){
		        strcat(err_string, Command[i].command);
		        strcat(err_string, " ");
		    }
	        }
	    }
	    return(put_error(err_string));
	}
    }
    else
	if (matches == 0) {
	    sprintf(err_string,"Unrecognized command: %s.", command_string);
	    return(put_error(err_string));
	}
    else {
	*new_command = '\0';
	(void) strcat(new_command, str);
	(void) strcat(new_command, " ");
	(void) strcat(new_command, Command[lastmatch].command);
    /* now that the new command string is done, call the function. */
	return((*Command[lastmatch].func)(new_command,Command[lastmatch].arg));
    }
}

/* comparison routine for sorting command lists -- jlm */

comcomp (p1, p2) char **p1, **p2; {
	int h1 = 0; /* high-level command, ends in '/' */
	int h2 = 0; 
	char *c1, *c2;
	char *e1, *e2;
	c1 = *p1;
	c2 = *p2;
	for (e1 = c1; *e1; e1++); e1--;
	for (e2 = c2; *e2; e2++); e2--;
	if (*e1 == '/') h1 = 1;
	if (*e2 == '/') h2 = 1;
	if (h1 > h2) return (-1);
	if (h2 > h1) return (1);
	return(strcmp(c1,c2));
}

/* ARGSUSED */
int lasthelpline = 1;

do_help(str, menu_type) char   *str; int     menu_type; {
    int     i,j,k;
    int	    foundone;
    int     len, listlen;
    int	    xpos;
    char    **tcommand;

    if (in_stream != stdin)
	return;

    clear_help();
    tcommand = ((char **) emalloc ((unsigned)((sizeof(char *)) * last_entry)));
    
    for (i = 0, j = 0; i < last_entry; i++) {
	if (Command[i].menutype == menu_type) {
		tcommand[j++] = Command[i].command;
	}
    }
    listlen = j;
    if (menu_type > 0) {
	qsort(tcommand,listlen,sizeof(char *),comcomp);
    }
    for (i = 0, xpos = 0; i < listlen; i++) {
    	    if (i && !strcmp(tcommand[i],tcommand[i-1]))
	    	continue;
	    len = strlen(tcommand[i]);
	    if ((xpos + 2 + len) > 79) {
		    io_move(++lasthelpline,0);
		    xpos = 0;
	    }
	    if (xpos == 0) {
		    io_printw("%s",tcommand[i]);
		    xpos += len;
	    }
	    else {
		    io_printw("  %s",tcommand[i]);
		    xpos += len + 2;
	    }
    }
    io_refresh();
    free(tcommand);
}


clear_help() {
    int     i;

    for (i = 1; i <= lasthelpline; i++) {
	io_move(i, 0);
	io_clrtoeol();
    }
    io_move(1, 0);
    io_refresh();
    lasthelpline = 1;
}

put_error(s)
char   *s;
{
    error_flag = 1;
    if(start_up) {
	 fprintf(stderr,"%s\n", s);
	 exit(0);
    }
    clear_help();
#ifdef MSDOS
    io_printw("Error: %s", s);
#else
    io_printw("Error: %s", s);
#endif
    scs();
    if (in_stream != stdin) return(file_error());
    return(CONTINUE);
}

scs() {
    io_refresh();
    sleep(3);
    clear_help();
    io_refresh();
}

init_commands() {
    extern struct Command_table *Command;
    int i;
    
    Command = (struct Command_table *)
              emalloc((unsigned)(maxcommands * sizeof(struct Command_table)));
    for (i = 0; i < maxcommands; i++) Command[i].command = 0;
}

/* declare this as void so we don't have to cast all calls to install_command 
	abh - 2/15/88 
*/

void install_command(str, func, menu, intp)
char   *str;
int     (*func)();
int     menu;
int    *intp;
{
    int     i,j;

    if (last_entry == maxcommands-1) {
        i = maxcommands;
	maxcommands += 20; /* increase maxcommands if necessary */
	Command = (struct Command_table *) erealloc((char *)Command, 
		   (unsigned)((maxcommands -20)*sizeof(struct Command_table)),
	           (unsigned)(maxcommands * sizeof(struct Command_table)));
	for (j = i; j < maxcommands; j++) Command[j].command = 0;
    }

    Command[last_entry].command = str;
    Command[last_entry].func = func;
    Command[last_entry].menutype = menu;
    Command[last_entry].arg = intp;

    last_entry++;
}

/* get_command and readline:  clear the command line, set the 
   command prompt, and get input from the screen.  the input 
   string from the screen is stored in the global string 
   Input_string[20].  This is  the pointer that get_command returns. */

static char Input_string[BUFSIZ];
static char Line_Buf[BUFSIZ];

#ifdef MSDOS
char *
readline() {
    char *lp = Line_Buf;
    char *rp;
    if (in_stream != stdin) {
        rp = fgets(Line_Buf,BUFSIZ,in_stream);
    }
    else {
	rp = gets(lp);
	rp[strlen(rp)] = '\n';
    }
    return (rp);
}
#else   (if not MSDOS)
char *
readline() {
    int ch;
    char *lp = Line_Buf;
    char * rp;
 
    if (in_stream != stdin) {
        rp = fgets(Line_Buf,BUFSIZ,in_stream);
    }

    else {
      rp = Line_Buf;
      while ( (ch = getc(in_stream)) != '\n' && ch != '\r' && ch != EOF) {
	if (ch == '\b' || ch == '\177') {
	  if (lp > Line_Buf) {
	    io_addch('');
	    io_addch(' ');
	    io_addch('');
	    io_refresh();
	    lp--;
	  }
	  continue;
	}
	*lp++ =ch;
	io_addch(ch);
	io_refresh();
      }
      *lp = '\n';
	*(lp + 1) = '\0';
    }
    return(rp);
}
#endif (not MSDOS)

char  *
get_command (prompt)
char  *prompt;
{
    static char *lp;
    char *ip = Input_string;

    if ( (file_err) || (endline && (in_stream == stdin))) {
	io_move(command_y, command_x);
	io_clrtoeol();
	io_refresh();
	io_move(command_y, command_x);
	io_printw("%s ", prompt);
	io_refresh();
	endline = 1;
    }
    if ( !endline && error_flag && (in_stream == stdin)) {
        /* the line contains an error so the rest is treated
	   as garbage */
	endline = 1;
	error_flag = 0;
	return ((char *) NULL);
    }
    
    error_flag = 0;
    
    if (endline) {
        lp = (char *) readline();
	if (lp == (char *) NULL) {
	    endline = 1;
	    return((char *) NULL);
	}
	else endline = 0;
    }

 /* eat all extra spaces */
    while (*lp == ' ' || *lp == '\t') lp++;
	if (*lp == EOF || *lp == '\n' || *lp == '\r') {
	endline = 1;
	return((char *) NULL);
    }


	while (*lp != ' ' && *lp != '\t' && *lp != '\n' && *lp != '\r') {
	    *ip++ = *lp++;
    }
    *ip = '\0';
    
    while (*lp == ' ' || *lp == '\t') *lp++;
    				/* eat all extra blanks */

	if (*lp == '\n' || *lp == '\r') {
	endline = 1;
    }

    return(Input_string);
}

char subprompt[40];
int intlevel = 0;

contin_test() {
    char *str;
    FILE *save_in_stream;
    
    save_in_stream = in_stream;

    if (in_stream != stdin) {
	in_stream = stdin;
    }
    endline = 1;
cont_again:
    str = get_command("p to push/b to break/<cr> to continue: ");
    if (str && *str == 'b') {
	in_stream = save_in_stream;
    	return(BREAK);
    }
    else if (str && *str == 'p') {
        sprintf(subprompt,"[%d] %s",++intlevel,Prompt);
	while (TRUE) {
        	if (do_command(subprompt, (int *) BASEMENU) == POP) break;
	}
	--intlevel;
	goto cont_again;
    }
    in_stream = save_in_stream;
/*  update_display(); */
    return(CONTINUE);
}

do_comfile() {
	char *str;
	char savename[BUFSIZ];
	int echeck,i;
	int nreps = 0;
	FILE *save_in_stream,*tmp_stream;
	
	
	str = get_command("command file name: ");
	if (!str) return(CONTINUE);

	strcpy(savename,str);
	
	if ( (tmp_stream = fopen_read_compat(str)) == NULL) {
		sprintf(err_string,"Cannot open %s.",str);
		return(put_error(err_string));
	}
	str = get_command("How many times? ");
	echeck = 0;
	if (str) echeck = sscanf(str,"%d",&nreps);
	if (echeck == 0) {
	  return(put_error("Integer argument missing in do command."));
	}
	save_in_stream = in_stream;
	in_stream = tmp_stream;
	for (i = 0; i < nreps; i++) {
	  while (!feof(in_stream)) {
	    if (do_command(Prompt, (int *) BASEMENU) == BREAK) {
		if (!feof(in_stream)) goto end_do_exec;
	    }
	  }
#ifdef MSDOS
	  if ( (in_stream = freopen(savename,"r",tmp_stream)) == NULL) {
		sprintf(err_string,"Cannot reopen %s.",savename);
		in_stream = save_in_stream;
		return(put_error(err_string));
	  }
#else	
	  fseek(in_stream,0L,0);
#endif
	}
end_do_exec:
	fclose(in_stream);
	in_stream = save_in_stream;
	return(CONTINUE);
}

file_error() {
    FILE *save_in_stream;
    char *str;
    save_in_stream = in_stream;
    in_stream = stdin;
    file_err = 1;
error_again:
    str = get_command("p to push/b to break/<cr> to continue: ");
    if (str && *str == 'b') {
	in_stream = save_in_stream;
	file_err = 0;
    	return(BREAK);
    }
    else if (str && *str == 'p') {
        sprintf(subprompt,"[%d] %s",++intlevel,Prompt);
	while (TRUE) {
        	if (do_command(subprompt, (int *) BASEMENU) == BREAK) break;
	}
	--intlevel;
	goto error_again;
    }
    in_stream = save_in_stream;
    file_err = 0;
    return(CONTINUE);
}
do_exec() {
	char *str;
	int echeck;
	char tbuf[BUFSIZ];
	
	str = get_command("command: ");
	if (!str) return(CONTINUE);
	strcpy(tbuf, str);
	while ((str = get_command("args: ")) != NULL) {
	    if (strcmp(str,"end") == 0) break;
	    strcat(tbuf," ");
	    strcat(tbuf,str);
	}
	system(tbuf);
}
