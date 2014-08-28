@echo OFF
SETLOCAL

REM
REM  DexCreateRelease.bat
REM

REM A batch file used to create timestamped archives of the GRIP scripts and pictures.

REM First parameter is the root name of the tar archives to be released.
set ROOT=%1

REM Second parameter is a qualifier: Debug, Draft or Release
set QUALIFIER=%2

REM Give a common name and time stamp to associate the three files.
set /A hour=%time:~0,2%+100
set TIMESTAMP=%ROOT% %QUALIFIER% (%date:~10,4%.%date:~4,2%.%date:~7,2% %hour:~1,2%.%time:~3,2%.%time:~6,2%)

REM Create the 'About' script with the current timestamp.
echo CMD_WAIT_SUBJ_READY,GRIP Scripts Version:\n\n%TIMESTAMP%,GripLogo.bmp,300 > task_about.dex

REM If this is a Debug release, we stop here. The simulator will execute in place.
if %QUALIFIER% EQU Debug goto END

REM Create a destination for the release tar files.
set DESTINATION=..\GripReleases\%TIMESTAMP%
mkdir "%Destination%"

REM Some constants
set LINT=..\DexLint\debug\DexLint.exe
set TAR="C:\Program Files\GnuWin32\bin\tar.exe"
set PICTURES=..\DexPictures
set PROOFS=..\GripProofs
set MD5="Z:\SoftwareDevelopement\DexterousManipulation\bin\MD5Tree.exe"

REM Only generate the proofs when it is a real release.
if %QUALIFIER% EQU Draft goto DRAFT
REM Don't do the proofs if it is a ground release.
if %ROOT% EQU GripGround goto DRAFT

%LINT%  -noquery -pictures=%PICTURES% users.dex -proofs=%PROOFS% -sbatch=CreateScriptsTar.bat -pbatch=CreatePicturesTar.bat -messages=MessageList.txt -log=Lint.log
%TAR% --create --verbose --file=Proofs.tar --directory=%PROOFS% *.bmp
copy /Y /V Proofs.tar "%DESTINATION%\%TIMESTAMP% Proofs.tar"
goto NEXT

:DRAFT
%LINT%  -noquery -pictures=%PICTURES% users.dex -sbatch=CreateScriptsTar.bat -pbatch=CreatePicturesTar.bat -messages=MessageList.txt -log=Lint.log
goto NEXT

:NEXT
call CreateScriptsTar.bat Scripts.tar
call CreatePicturesTar.bat Pictures.tar

echo %TIMESTAMP% > relnotes1
copy /b relnotes1+..\DexSourceCode\GripReleaseNotes.txt relnotes2
rename ..\DexSourceCode\GripReleaseNotes.txt GripReleaseNotes.bak
copy relnotes2 ..\DexSourceCode\GripReleaseNotes.txt

copy /Y /V Scripts.tar  "%DESTINATION%\%TIMESTAMP% Scripts.tar"
copy /Y /V Pictures.tar "%DESTINATION%\%TIMESTAMP% Pictures.tar"
copy /Y /V ..\DexSourceCode\GripInstallationInstructions.txt "%DESTINATION%\%TIMESTAMP% Installation Note.txt" 
copy /Y /V ..\DexSourceCode\GripReleaseNotes.txt "%DESTINATION%\%TIMESTAMP% Release Notes.txt" 

REM Here we compute the checksums for the release.
pushd %DESTINATION%
%MD5% *.tar *.txt > "%TIMESTAMP%.md5"
popd
echo %TIMESTAMP% > ..\DexSourceCode\%ROOT%.release

:END
