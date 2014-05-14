@echo off
SETLOCAL


REM Generate the Dynamics Flight Reduced protocol. (From a copy of the DexGenerateDynamics.bat)

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
REM  start numbering from different values, i.e. Dynamics = 200, Upright = 300, Supine = 400, Reduced = 500.
set task=500

REM Write a header into the file.
ECHO #DEX protocol file for Dynamics Flight Reduced protocol.
ECHO #Generated by %0:  %date% %time%
ECHO #Format: "CMD_TASK", task id, task file, display name


REM ****************************************************************************

REM For common tasks we assume that the scripts have already been constructed, so we just create the appropriate lines in the procedure file.

REM ****************************************************************************

REM Standard tasks at the start of a subsession.

REM Perform the install of the equipment in the upright (seated) position.
REM Each subject should do this, even the configuration has changed, to be sure that the CODAs are aligned.
call %SOURCE%\DexCreateInstallTask.bat Upright

REM Make sure that the audio is set loud enough. Also tells subject to strap in.
set /A "task=task+1"
echo CMD_TASK,%task%,TaskCheckAudio.dex,%task% Check Audio

REM The force sensor offsets are also measured and suppressed at the start for each subject.
call %SOURCE%\DexCreateOffsetTask.bat -deploy

REM ****************************************************************************

REM
REM Coefficient of Friction tests.
REM

REM Start the trial counter for the friction tests.
set fric_seq=0

call %SOURCE%\DexCreateFrictionTask.bat 2.5 -prep
call %SOURCE%\DexCreateFrictionTask.bat 1.0
call %SOURCE%\DexCreateFrictionTask.bat 0.5 -stow

REM ****************************************************************************

REM Now we start generating the science task scripts specific to this protocol.
REM Here we build the scripts as we go and we create the entries in the protocol file.


REM ****************************************************************************

REM
REM Discrete Movements
REM

REM All the trials mass.
set mass=400gm

REM Start the trial counter for the oscillations.
set dsc_seq=0

set direction=Vertical
set eyes=open  
set range=DiscreteRangesVertical.txt:%sz%
call %SOURCE%\DexCreateDiscreteTask.bat -prep

set direction=Vertical
set eyes=closed
set range=DiscreteRangesVertical.txt:%sz%
call %SOURCE%\DexCreateDiscreteTask.bat

set direction=Horizontal
set eyes=open  
set range=DiscreteRangesHorizontal.txt:%sz%
call %SOURCE%\DexCreateDiscreteTask.bat -prep

set direction=Horizontal
set eyes=closed
set range=DiscreteRangesHorizontal.txt:%sz%
call %SOURCE%\DexCreateDiscreteTask.bat

REM ****************************************************************************

REM
REM Oscillations
REM

REM All the oscillations are in the same direction and of the same duration.
set direction=Vertical
set dir=%direction:~0,4%
set duration=30.0

REM Start the trial counter for the oscillations.
set osc_seq=100

set mass=600gm
set frequency=1.00
set prep=-prep
set range=OscillationRangesNominalVertical.txt:%sz%
call %SOURCE%\DexCreateOscillationTask.bat

set mass=600gm
set frequency=1.00
set prep=
set range=OscillationRangesNominalVertical.txt:%sz%
call %SOURCE%\DexCreateOscillationTask.bat

REM ****************************************************************************


REM
REM Collisions
REM

set mass=600gm
set nblocks=5
call %SOURCE%\DexCreateCollisionTasks.bat

REM ****************************************************************************

REM
REM Targeted Movements
REM

set mass=800gm
set nblocks=5

REM
REM Vertical Direction
REM
set direction=Vertical
call %SOURCE%\DexCreateTargetedTasks.bat

REM ***************************************************************************

REM
REM Coefficient of Friction tests.
REM

call %SOURCE%\DexCreateFrictionTask.bat 1.0 -deploy -prep

REM ****************************************************************************

REM
REM Show that protocol is finished.
REM
set /A "task=task+1"
echo CMD_TASK,%task%,TaskFinishProtocol.dex,(Done - press Back)

ENDLOCAL
goto :EOF

