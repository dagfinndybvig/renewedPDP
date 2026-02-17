/*

       This file is part of the PDP software package.
		 
       Copyright 1987 by James L. McClelland and David E. Rumelhart.
       
       Please refer to licensing information in the file license.txt,
       which is in the same directory with this source file and is
       included here by reference.
*/


/* file: iatop.c

	User interface for the ia program.
	
	First version implemented by Elliot Jaffe.
	
	Date of last revision:  8-12-87/JLM.
*/

#include "general.h"
#include "cs.h"
#include "variable.h"
#include "command.h"
#include "ia.h"

char *Prompt = "ia: ";
char *Default_step_string = "cycle";

newstart() {
    random_seed = rand();
    reset();
}

reset() {
    zarrays();
    update_out_values();
    clear_display();
    update_display();
}

init_system()
{
   int trial(),reset(),cycle(), printout(), get_disp_list(),
       oword_list(),oletter_list(),clear_list(),fc();

   Display_level = 1;

   install_command("trial",trial, BASEMENU, (int *)NULL);
   install_command("fcspec",fc, BASEMENU, (int *)NULL);
   install_command("reset",reset, BASEMENU, (int *)NULL);
   install_command("newstart",newstart, BASEMENU, (int *)NULL);
   install_command("cycle",cycle, BASEMENU, (int *)NULL);
   install_command("print",printout, BASEMENU, (int *)NULL);

   install_command("dlist",get_disp_list, GETMENU, (int *)NULL);
   install_var("wthresh",Float,(int *)& dthresh[W],0,0,DISPLAYOPTIONS);
   install_var("lthresh",Float,(int *)& dthresh[L],0,0,DISPLAYOPTIONS);

   install_var("ncycles",Int, (int *)& ncycles,0,0, SETPCMENU);
   install_var("comprp",Int, (int *)& compute_resprob,0,0, SETMODEMENU);
   
   install_command("alpha/", do_command,SETPARAMMENU, (int *)AlphaMenu);
   install_var("f->l",Float,(int *)& a[1], 0,0, AlphaMenu);
   install_var("l->w",Float,(int *)& a[4], 0,0, AlphaMenu);
   install_var("w->l",Float,(int *)& a[5], 0,0, AlphaMenu);

   install_command("beta/", do_command, SETPARAMMENU, (int *)BetaMenu);
   install_var("letter",Float,(int *)& b[3], 0,0, BetaMenu);
   install_var("word",Float,(int *)& b[6], 0,0, BetaMenu);

   install_command("gamma/", do_command, SETPARAMMENU,(int *)GammaMenu);
   install_var("f->l",Float,(int *)& g[1], 0,0, GammaMenu);
   install_var("l->l",Float,(int *)& g[3], 0,0, GammaMenu);
   install_var("l->w",Float,(int *)& g[4], 0,0, GammaMenu);
   install_var("w->l",Float,(int *)& g[5], 0,0, GammaMenu);
   install_var("w->w",Float,(int *)& g[6], 0,0, GammaMenu);

   install_command("thresh/",do_command,SETPARAMMENU,(int *)ThreshMenu);
   install_var("letter",Float,(int *)& t[3], 0,0, ThreshMenu);
   install_var("w->l",Float,(int *)& t[6], 0,0, ThreshMenu);
   install_var("w->w",Float,(int *)& t[6], 0,0, ThreshMenu);

   install_command("max/", do_command, SETPARAMMENU, (int *)MaxMenu);
   install_var("letter",Float,(int *)& max[3], 0,0, MaxMenu);
   install_var("word",Float,(int *)& max[6], 0,0, MaxMenu);

   install_command("min/", do_command, SETPARAMMENU, (int *)MinMenu);
   install_var("letter",Float,(int *)& min[3], 0,0, MinMenu);
   install_var("word",Float,(int *)& min[6], 0,0, MinMenu);

   install_command("rest/", do_command, SETPARAMMENU, (int *)RestMenu);
   install_var("letter",Float,(int *)& rest[3], 0,0, RestMenu);
   install_var("word",Float,(int *)& rest[6], 0,0, RestMenu);

   install_command("oscale/",do_command,SETPARAMMENU,(int*)OscaleMenu);
   install_var("letter",Float,(int *)& oscale[3], 0,0, OscaleMenu);
   install_var("word",Float,(int *)& oscale[6], 0,0, OscaleMenu);

   install_command("fdprob/",do_command,SETPARAMMENU,(int *)ProbMenu);
   install_var("f0",Float,(int *)& fdprob[0], 0,0, ProbMenu);
   install_var("f1",Float,(int *)& fdprob[1], 0,0, ProbMenu);
   install_var("f2",Float,(int *)& fdprob[2], 0,0, ProbMenu);
   install_var("f3",Float,(int *)& fdprob[3], 0,0, ProbMenu);
   install_var("f4",Float,(int *)& fdprob[4], 0,0, ProbMenu);
   install_var("f5",Float,(int *)& fdprob[5], 0,0, ProbMenu);
   install_var("f6",Float,(int *)& fdprob[6], 0,0, ProbMenu);

   install_command("estr/",do_command,SETPARAMMENU,(int *)EstrMenu);
   install_var("p0",Float,(int *)& estr[0], 0,0, EstrMenu);
   install_var("p1",Float,(int *)& estr[1], 0,0, EstrMenu);
   install_var("p2",Float,(int *)& estr[2], 0,0, EstrMenu);
   install_var("p3",Float,(int *)& estr[3], 0,0, EstrMenu);
   
   install_var("fgain",Float,(int *)& fgain, 0,0, SETPARAMMENU);
   install_var("orate",Float,(int *)& outrate, 0,0, SETPARAMMENU);

   install_var("cycleno",Int,(int *)& cycleno, 0,0, SETSVMENU);
   install_var("nwords",Int,(int *)& tally, 0,0, NOMENU);
   install_var("awords",Float,(int *)& prsum, 0,0, NOMENU);
   install_var("nlpos0",Int,(int *)& tal[0], 0,0,NOMENU);
   install_var("nlpos1",Int,(int *)& tal[1], 0,0,NOMENU);
   install_var("nlpos2",Int,(int *)& tal[2], 0,0,NOMENU);
   install_var("nlpos3",Int,(int *)& tal[3], 0,0,NOMENU);
   install_var("alpos0",Float,(int *)& prsm[0], 0,0,NOMENU);
   install_var("alpos1",Float,(int *)& prsm[1], 0,0,NOMENU);
   install_var("alpos2",Float,(int *)& prsm[2], 0,0,NOMENU);
   install_var("alpos3",Float,(int *)& prsm[3], 0,0,NOMENU);
   install_var("fcpos",Int,(int *)& fc_pos, 0,0,NOMENU);
   install_var("fcmax",Float,(int *)& fc_max, 0,0,NOMENU);
   install_var("trial",Vstring,(int *)trial_history, 0,0,NOMENU);
   install_var("dwp",Vstring,(int *)disp_word_ptr,0,0,NOMENU);
   install_var("dwa",Vfloat,(int *)disp_word_act,0,0,NOMENU);
   install_var("dwr",Vfloat,(int *)disp_word_rpr,0,0,NOMENU);
   install_var("dlp0",Vstring,(int *)disp_let_ptr[0],0,0,NOMENU);
   install_var("dla0",Vfloat,(int *)disp_let_act[0],0,0,NOMENU); 
   install_var("dlr0",Vfloat,(int *)disp_let_rpr[0],0,0,NOMENU);
   install_var("dlp1",Vstring,(int *)disp_let_ptr[1],0,0,NOMENU);
   install_var("dla1",Vfloat,(int *)disp_let_act[1],0,0,NOMENU);
   install_var("dlr1",Vfloat,(int *)disp_let_rpr[1],0,0,NOMENU);
   install_var("dlp2",Vstring,(int *)disp_let_ptr[2],0,0,NOMENU);
   install_var("dla2",Vfloat,(int *)disp_let_act[2],0,0,NOMENU);
   install_var("dlr2",Vfloat,(int *)disp_let_rpr[2],0,0,NOMENU);
   install_var("dlp3",Vstring,(int *)disp_let_ptr[3],0,0,NOMENU);
   install_var("dla3",Vfloat,(int *)disp_let_act[3],0,0,NOMENU);
   install_var("dlr3",Vfloat,(int *)disp_let_rpr[3],0,0,NOMENU);
   install_var("dfp",Vstring,(int *)disp_fc_ptr,2,0,NOMENU);
   install_var("dfa",Vfloat,(int *)disp_fc_act,2,0,NOMENU);
   install_var("dfr",Vfloat,(int *)disp_fc_rpr,2,0,NOMENU);
    zarrays();
}
