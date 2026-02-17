ECHO OFF
REM Copyright (c) 1987 by McClelland and Rumelhart
REM Permission is granted to modify and reproduce this file for personal use.
REM
CLS
ECHO PDPLINK
ECHO Command file for (re)linking object files supplied with the
ECHO handbook "Exploring Parallel Distributed Processing", under
ECHO the Microsoft (R) C Compiler (compiler version 3.0 or later)
ECHO .
ECHO .
BREAK ON
IF '%1' == 'PLOT' GOTO PLOT
IF '%1' == 'plot' GOTO PLOT
IF '%1' == 'COLEX' GOTO COLEX
IF '%1' == 'colex' GOTO COLEX
IF EXIST libpc.a GOTO START
ECHO The library file libpc.a is missing from this directory.  Use
ECHO the MAKELIB (batch-file) command to make the library before linking.
GOTO QUIT
:START
IF '%1' == 'ALL' GOTO ALL
IF '%1' == 'AA' GOTO AA
IF '%1' == 'BP' GOTO BP
IF '%1' == 'CL' GOTO CL
IF '%1' == 'CS' GOTO CS
IF '%1' == 'IA' GOTO IA
IF '%1' == 'IAC' GOTO IAC
IF '%1' == 'PA' GOTO PA
IF '%1' == 'all' GOTO ALL
IF '%1' == 'aa' GOTO AA
IF '%1' == 'bp' GOTO BP
IF '%1' == 'cl' GOTO CL
IF '%1' == 'cs' GOTO CS
IF '%1' == 'ia' GOTO IA
IF '%1' == 'iac' GOTO IAC
IF '%1' == 'pa' GOTO PA
ECHO Wrong form for command.
ECHO Type PDPLINK X where X is one of AA,BP,CL,CS,IA,IAC,PA,PLOT,COLEX or ALL.
ECHO All object files associated with the program named will then be linked.
GOTO END
:ALL
:AA
ECHO Linking AA.
LINK aa, , , libpc.a ; 
EXEMOD aa /stack f00
IF '%1' == 'ALL' GOTO BP
IF '%1' == 'all' GOTO BP
GOTO END
:BP
ECHO Linking BP.
LINK bp+weights, , , libpc.a ; 
EXEMOD bp /stack f00
IF '%1' == 'ALL' GOTO CL
IF '%1' == 'all' GOTO CL
GOTO END
:CL
ECHO Linking CL.
LINK cl, , , libpc.a ; 
EXEMOD cl /stack f00
IF '%1' == 'ALL' GOTO CS
IF '%1' == 'all' GOTO CS
GOTO END
:CS
ECHO Linking CS.
LINK cs+weights, , , libpc.a ; 
EXEMOD cs /stack f00
IF '%1' == 'ALL' GOTO IA
IF '%1' == 'all' GOTO IA
GOTO END
:IA
ECHO Linking IA.
LINK ia+iaaux+iatop, , , libpc.a ; 
EXEMOD ia /stack f00
IF '%1' == 'ALL' GOTO IAC
IF '%1' == 'all' GOTO IAC
GOTO END
:IAC
ECHO Linking IAC.
LINK iac+weights, , , libpc.a ; 
EXEMOD iac /stack f00
IF '%1' == 'ALL' GOTO PA
IF '%1' == 'all' GOTO PA
GOTO END
:PA
ECHO Linking PA.
LINK pa+weights, , , libpc.a ; 
EXEMOD pa /stack f00
GOTO END
:PLOT
LINK plot ;
GOTO END
:COLEX
LINK colex ;
:END
ECHO Finished linking.
:QUIT
