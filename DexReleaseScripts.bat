@echo ON
SETLOCAL

REM
REM  DexCreateRelease.bat
REM

REM A batch file used to create timestamped archives of the GRIP scripts and pictures.

REM First parameter is the root name of the tar archives to be released.
set ROOT=%1

REM Destination is a constant for the moment.
set DESTINATION=..\GripReleases

REM Give a common name and time stamp to associate the three files.
set TIMESTAMP=GRIP Release (%date:~10,4%.%date:~7,2%.%date:~4,2% %time:~0,2%.%time:~3,2%.%time:~6,2%)

REM Each release consists of a script tar file, a pictures tar file and md5 checks for each of them.
REM It also contains the installation instructions.
copy /Y /V %ROOT%Scripts.tar  "%DESTINATION%\%TIMESTAMP% %ROOT%Scripts.tar"
copy /Y /V %ROOT%Pictures.tar "%DESTINATION%\%TIMESTAMP% %ROOT%Pictures.tar"
copy /Y /V %ROOT%.md5         "%DESTINATION%\%TIMESTAMP% %ROOT%.md5" 
copy /Y /V ..\DexSourceCode\GripInstallationInstructions.txt "%DESTINATION%\%TIMESTAMP% Installation Note.txt" 
copy /Y /V ..\DexSourceCode\GripReleaseNotes.txt "%DESTINATION%\%TIMESTAMP% Release Notes.txt" 
