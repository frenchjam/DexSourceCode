@echo on
SETLOCAL

REM
REM  DexSubjects.bat
REM

REM A batch file used to create the users.dex files and associated session files.

REM First parameter is the qualifier used in the session filenames (Flight or Ground).
set QUALIFIER=%1

REM Second is the output filename
set OUTPUT=%2

echo # DEX user file for FLIGHT models. > temp1
echo # Format: CMD_USER, user %QUALIFIER%, pin code, session file, display name >> temp1
echo # Subject IDs:  10-19 small subjects; 20-29 medium subjects; 30-39 large subjects. >> temp1
echo # Subject codes should be the same between Flight and Ground.  >> temp1
echo # It's the session files that change between models. >> temp1

echo CMD_USER,11,138,S1SessionSmallSubject%QUALIFIER%.dex,Subject 1  >> temp1
echo CMD_PROTOCOL,200,ignore,********* Subject 1 ********* > temp2
copy temp2+SessionSmallSubject%QUALIFIER%.dex S1SessionSmallSubject%QUALIFIER%.dex

echo CMD_USER,12,467,S2SessionSmallSubject%QUALIFIER%.dex,Subject 2  >> temp1
echo CMD_PROTOCOL,200,ignore,********* Subject 2 ********* > temp2
copy temp2+SessionSmallSubject%QUALIFIER%.dex S2SessionSmallSubject%QUALIFIER%.dex

echo CMD_USER,23,941,S3SessionMediumSubject%QUALIFIER%.dex,Subject 3  >> temp1
echo CMD_PROTOCOL,200,ignore,********* Subject 3 ********* > temp2
copy temp2+SessionMediumSubject%QUALIFIER%.dex S3SessionMediumSubject%QUALIFIER%.dex

echo CMD_USER,24,510,S4SessionMediumSubject%QUALIFIER%.dex,Subject 4  >> temp1
echo CMD_PROTOCOL,200,ignore,********* Subject 4 ********* > temp2
copy temp2+SessionMediumSubject%QUALIFIER%.dex S4SessionMediumSubject%QUALIFIER%.dex

echo CMD_USER,25,301,S5SessionMediumSubject%QUALIFIER%.dex,Subject 5  >> temp1
echo CMD_PROTOCOL,200,ignore,********* Subject 5 ********* > temp2
copy temp2+SessionMediumSubject%QUALIFIER%.dex S5SessionMediumSubject%QUALIFIER%.dex

echo CMD_USER,36,931,S6SessionLargeSubject%QUALIFIER%.dex,Subject 6  >> temp1
echo CMD_PROTOCOL,200,ignore,********* Subject 6 ********* > temp2
copy temp2+SessionLargeSubject%QUALIFIER%.dex S6SessionLargeSubject%QUALIFIER%.dex

echo CMD_USER,37,036,S7SessionLargeSubject%QUALIFIER%.dex,Subject 7  >> temp1
echo CMD_PROTOCOL,200,ignore,********* Subject 7 ********* > temp2
copy temp2+SessionLargeSubject%QUALIFIER%.dex S7SessionLargeSubject%QUALIFIER%.dex

echo CMD_USER,38,483,S8SessionLargeSubject%QUALIFIER%.dex,Subject 8  >> temp1
echo CMD_PROTOCOL,200,ignore,********* Subject 8 ********* > temp2
copy temp2+SessionLargeSubject%QUALIFIER%.dex S8SessionLargeSubject%QUALIFIER%.dex


echo CMD_USER,99,987,SessionUtilitiesOnly.dex,Maintenance  >> temp1

del /F /Q %OUTPUT%
rename temp1 %OUTPUT%