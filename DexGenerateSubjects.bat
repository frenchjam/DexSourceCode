@echo off
SETLOCAL

REM
REM  DexGenerateSubjects.bat
REM

REM A batch file used to create the users.dex files and associated session files.

REM First parameter is the qualifier used in the session filenames (Flight or BDC).
set QUALIFIER=%1
set Q=%QUALIFIER:~0,2%

REM Second is the output filename
set OUTPUT=%2

echo # DEX user file for FLIGHT models. > temp1
echo # Format: CMD_USER, user %QUALIFIER%, pin code, session file, display name >> temp1
echo # Subject IDs:  10-19 small subjects; 20-29 medium subjects; 30-39 large subjects. >> temp1
echo # Subject codes should be the same between Flight and Ground.  >> temp1
echo # It's the session files that change between models. >> temp1

if /I %QUALIFIER% EQU Flight goto TESTS

set fn=Demo.dex
echo CMD_USER,98,000,%fn%,Training DEMO  >> temp1
echo CMD_PROTOCOL,200,ignore,*** Training DEMOS *** > temp2
copy temp2+SessionDemo.dex %fn% 

set fn=SsSml%Q%.dex
echo CMD_USER,19,123,%fn%,Training S  >> temp1
echo CMD_PROTOCOL,200,ignore,***** Training S ***** > temp2
copy temp2+SessionSmallSubjectTraining.dex %fn% 

set fn=SmMed%Q%.dex
echo CMD_USER,29,456,%fn%,Training M  >> temp1
echo CMD_PROTOCOL,200,ignore,***** Training M ***** > temp2
copy temp2+SessionMediumSubjectTraining.dex %fn% 

set fn=SlLrg%Q%.dex
echo CMD_USER,39,789,%fn%,Training L  >> temp1
echo CMD_PROTOCOL,200,ignore,***** Training L ***** > temp2
copy temp2+SessionLargeSubjectTraining.dex %fn% 


:TESTS

set fn=S1Sml%Q%.dex
echo CMD_USER,11,138,%fn%,BDC Subject 1  >> temp1
echo CMD_PROTOCOL,200,ignore,***** Subject 1 ***** > temp2
copy temp2+SessionSmallSubject%QUALIFIER%.dex %fn% 

set fn=S2Sml%Q%.dex
echo CMD_USER,12,467,%fn%,BDC Subject 2  >> temp1
echo CMD_PROTOCOL,200,ignore,***** Subject 2 ***** > temp2
copy temp2+SessionSmallSubject%QUALIFIER%.dex %fn% 

set fn=S3Med%Q%.dex
echo CMD_USER,23,941,%fn%,BDC Subject 3  >> temp1
echo CMD_PROTOCOL,200,ignore,***** Subject 3 ***** > temp2
copy temp2+SessionMediumSubject%QUALIFIER%.dex %fn% 

set fn=S4SMed%Q%.dex
echo CMD_USER,24,510,%fn%,BDC Subject 4  >> temp1
echo CMD_PROTOCOL,200,ignore,***** Subject 4 ***** > temp2
copy temp2+SessionMediumSubject%QUALIFIER%.dex %fn% 

set fn=S5Med%Q%.dex
echo CMD_USER,25,301,%fn%,BDC Subject 5  >> temp1
echo CMD_PROTOCOL,200,ignore,***** Subject 5 ***** > temp2
copy temp2+SessionMediumSubject%QUALIFIER%.dex %fn% 

set fn=S6Lrg%Q%.dex
echo CMD_USER,36,931,%fn%,BDC Subject 6  >> temp1
echo CMD_PROTOCOL,200,ignore,***** Subject 6 ***** > temp2
copy temp2+SessionLargeSubject%QUALIFIER%.dex %fn% 

set fn=S7Lrg%Q%.dex
echo CMD_USER,37,036,%fn%,BDC Subject 7  >> temp1
echo CMD_PROTOCOL,200,ignore,***** Subject 7 ***** > temp2
copy temp2+SessionLargeSubject%QUALIFIER%.dex %fn% 

set fn=S8Lrg%Q%.dex
echo CMD_USER,38,483,%fn%,BDC Subject 8  >> temp1
echo CMD_PROTOCOL,200,ignore,***** Subject 8 ***** > temp2
copy temp2+SessionLargeSubject%QUALIFIER%.dex %fn% 

if /I %QUALIFIER% EQU Flight goto MAINTENANCE

:MAINTENANCE

REM set fn=SsMedEvalFlight.dex
REM echo CMD_USER,97,111,%fn%,Eval  >> temp1
REM echo CMD_PROTOCOL,200,ignore,***** Eval ***** > temp2
REM copy temp2+SessionFlightOnGround.dex %fn% 

echo CMD_USER,99,987,SessionU.dex,Maintenance  >> temp1

del /F /Q %OUTPUT%
rename temp1 %OUTPUT%