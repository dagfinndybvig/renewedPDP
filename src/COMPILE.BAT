ECHO OFF
CLS
IF '%1' == 'ALL' GOTO ALL
IF '%1' == 'all' GOTO ALL
REM if not compiling all programs, assume that the argument %1 is the file
REM to compile
REM
ECHO Now compiling %1.
CL -AL -Ot -FPi -Gt %1 /c /D MSDOS
GOTO END
:ALL
ECHO Now compiling all files *.c in this directory.
FOR %%f IN (*.c) DO CL -AL -Ot -FPi -Gt %%f /c /D MSDOS
:END
ECHO Finished.
