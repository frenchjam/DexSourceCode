@echo off
SETLOCAL

REM Generate the Referential Flight protocol.

REM This script allows one to select -upright or -supine with the first parameter.
set posture=%1
set pstr=%posture:~0,2%

REM This script takes an additional input parameter, which can be sml, med, lrg. It adds this as a modifier to the target file list
REM so that the targets can be tuned to different size subject. It also inserts this string into the script file name
set size=%2
set sz=%size:~0,1%

REM Alias to the compiler.
set COMPILER=DexSimulatorApp.exe
set SOURCE=..\DexSourceCode

REM Task Counter
REM Here I am adopting a strategy by which all the protocols of the same type but for different size subjects
REM  (small, medium and large) will use the same ID number for each task, but that the different protocols will 
REM  start numbering from different values, i.e. Dynamics = 200, Upright = 300, Supine = 400.
set task=300
if /I %posture% EQU supine SET task=400 

REM Write a header into the file.
ECHO #DEX protocol file for Reference Frame Flight protocol.
ECHO #Generated by %0:  %date% %time%
ECHO #Format: "CMD_TASK", task id, task file, display name


REM ****************************************************************************

REM For common tasks we assume that the scripts have already been constructed.
REM So here we just create the appropriate lines in the procedure file.

REM For the science trials we start generating the science task scripts specific 
REM at the same time that we create the entries in the protocol file.

REM ****************************************************************************

REM Standard tasks at the start of a subsession.

REM Perform the install of the equipment in the upright (seated) position.
REM Each subject should do this, even the configuration has changed, to be sure that the CODAs are aligned.
if /I %posture% EQU supine GOTO :SUPINE 
call %SOURCE%\DexCreateInstallTask.bat Upright
GOTO :NEXT
:SUPINE
call %SOURCE%\DexCreateInstallTask.bat Supine
:NEXT

REM Make sure that the audio is set loud enough. Also tells subject to strap in.
set /A "task=task+1"
echo CMD_TASK,%task%,TaskCheckAudio.dex,%task% Check Audio

REM The force sensor offsets are also measured and suppressed at the start for each subject.
call %SOURCE%\DexCreateOffsetTask.bat -deploy

REM ****************************************************************************

REM
REM Coefficient of Friction tests.
REM

call %SOURCE%\DexCreateFrictionTask.bat 2.5 -prep
call %SOURCE%\DexCreateFrictionTask.bat 1.0
call %SOURCE%\DexCreateFrictionTask.bat 0.5 -stow

REM ****************************************************************************

REM
REM Discrete Movements
REM

REM All the trials have the same range, frequency, duration and mass.
set mass=400gm

REM Start the trial counter for the discrete movements.
set dsc_seq=0

set direction=Vertical
set eyes=open  
set range=DiscreteRanges%direction%.txt:%sz%
call %SOURCE%\DexCreateDiscreteTask.bat -prep

set direction=Vertical
set eyes=closed
set range=DiscreteRanges%direction%.txt:%sz%
call %SOURCE%\DexCreateDiscreteTask.bat 

set direction=Vertical
set eyes=open  
set range=DiscreteRanges%direction%.txt:%sz%
call %SOURCE%\DexCreateDiscreteTask.bat 

set direction=Vertical
set eyes=closed
set range=DiscreteRanges%direction%.txt:%sz%
call %SOURCE%\DexCreateDiscreteTask.bat 

set direction=Horizontal
set eyes=open  
set range=DiscreteRanges%direction%.txt:%sz%
call %SOURCE%\DexCreateDiscreteTask.bat -prep

set direction=Horizontal
set eyes=closed
set range=DiscreteRanges%direction%.txt:%sz%
call %SOURCE%\DexCreateDiscreteTask.bat 

set direction=Horizontal
set eyes=open  
set range=DiscreteRanges%direction%.txt:%sz%
call %SOURCE%\DexCreateDiscreteTask.bat 

set direction=Horizontal
set eyes=closed
set range=DiscreteRanges%direction%.txt:%sz%
call %SOURCE%\DexCreateDiscreteTask.bat 


REM ****************************************************************************

REM
REM Collisions
REM
set mass=600gm
set nblocks=10
call %SOURCE%\DexCreateCollisionTasks.bat

set mass=400gm
set nblocks=10
call %SOURCE%\DexCreateCollisionTasks.bat

REM ****************************************************************************

REM
REM Coefficient of Friction tests.
REM

call %SOURCE%\DexCreateFrictionTask.bat 2.5 -prep -deploy
call %SOURCE%\DexCreateFrictionTask.bat 1.0
call %SOURCE%\DexCreateFrictionTask.bat 0.5

REM ****************************************************************************

REM
REM Show that protocol is finished.
REM
set /A "task=task+1"
echo CMD_TASK,%task%,TaskFinishProtocol.dex,Finished - press Back

ENDLOCAL
goto :EOF

