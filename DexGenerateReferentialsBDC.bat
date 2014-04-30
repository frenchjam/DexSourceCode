@echo off
SETLOCAL


REM Generate the Referential BDC protocol.

REM This script allows one to select -upright or -supine with the first parameter.
set posture=%1
set pstr=%posture:~0,2%

REM This script takes an additional input parameter, which can be sml, med, lrg. It adds this as a modifier to the target file list
REM so that the targets can be tuned to different size subject. It also inserts this string into the script file name
set size=%2
set sz=%size:~0,1%

REM Alias to the compiler.
set COMPILER=DexSimulatorApp.exe

REM Task Counter
REM Here I am adopting a strategy by which all the protocols of the same type but for different size subjects
REM  (small, medium and large) will use the same ID number for each task, but that the different protocols will 
REM  start numbering from different values, i.e. Dynamics = 200, Upright = 300, Supine = 400.
set task=700
if /I %posture% EQU supine SET task=800 

REM Write a header into the file.
ECHO #DEX protocol file for Reference Frame BDC protocol.
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
set /A "task=task+1"
if /I %posture% EQU supine GOTO :SUPINE 
echo CMD_TASK,%task%,InstallUprightTask.dex,%task% Configure
GOTO :NEXT
:SUPINE
echo CMD_TASK,%task%,InstallSupineTask.dex,%task% Configure
:NEXT

REM The force sensor offsets are also measured and suppressed at the start for each subject.
set /A "task=task+1"
echo CMD_TASK,%task%,ForceOffsets.dex,%task% Cancel Offsets

REM ****************************************************************************

REM
REM Coefficient of Friction tests.
REM
REM !!! Need to define how many to be done and at what desired grip force.
REM !!! We also need to decide on the method.

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest0p5.dex,%task% Friction 0.5

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest1p0.dex,%task% Friction 1.0

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest2p5.dex,%task% Friction 2.5


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
set range=DiscreteRangesVertical.txt:%sz% -prep
call :DO_ONE_DISCRETE_TRIAL

set direction=Vertical
set eyes=closed
set range=DiscreteRangesVertical.txt:%sz%
call :DO_ONE_DISCRETE_TRIAL

set direction=Horizontal
set eyes=open
set range=DiscreteRangesHorizontal.txt:%sz% -prep
call :DO_ONE_DISCRETE_TRIAL

set direction=Horizontal
set eyes=closed
set range=DiscreteRangesHorizontal.txt:%sz%
call :DO_ONE_DISCRETE_TRIAL

set direction=Vertical
set eyes=open
set range=DiscreteRangesHorizontal.txt:%sz%
call :DO_ONE_DISCRETE_TRIAL

set direction=Vertical
set eyes=closed
set range=DiscreteRangesVertical.txt:%sz%
call :DO_ONE_DISCRETE_TRIAL

set direction=Horizontal
set eyes=open
set range=DiscreteRangesVertical.txt:%sz%
call :DO_ONE_DISCRETE_TRIAL

set direction=Horizontal
set eyes=closed
set range=DiscreteRangesHorizontal.txt:%sz%
call :DO_ONE_DISCRETE_TRIAL


REM ****************************************************************************

REM
REM Collisions
REM

set mass=400gm
set nblocks=10
call :DO_BLOCK_OF_COLLISIONS

REM ****************************************************************************


REM
REM Coefficient of Friction tests.
REM

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest0p5.dex,%task% Friction 0.5

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest1p0.dex,%task% Friction 1.0

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest2p5.dex,%task% Friction 2.5

ENDLOCAL
goto :EOF


REM ****************************************************************************

REM A subroutine to construct a trial of discrete movements (task).
REM It assumes that the task and seq counters were initialized and
REM  that direction, mass and range have been set.

:DO_ONE_DISCRETE_TRIAL

	set /A "task=task+1"
	set /A "dsc_seq=dsc_seq+1"
	set dir=%direction:~0,4%
	set filename=GndDsc%pstr%%dir%%mass%%size%%dsc_seq%.dex
	%COMPILER% -discrete -%mass% -%posture% -%direction% -range=%range% -delays=DiscreteDelaySequences30.txt:1 -compile=%filename% -%eyes% 
	echo CMD_TASK,%task%,%filename%,%task% Discrete %dsc_seq%
	goto :EOF

REM ****************************************************************************

REM Subroutines to generate a block of targeted movement trials.
REM The first initializes parameters for multiple blocks of trials.
REM It calls the second one which generates the commands for each block (task).

:DO_BLOCK_OF_COLLISIONS

	REM Provide instructions for first block.
	REM Subroutine will disable it for subsequent blocks.
	set prep=-prep

	REM Initialize the block counter. 
	REM It gets incremented by the subroutine.
	set seq=0

	REM Procuce the required number of blocks.
	FOR /L %%s IN ( 1,1,%nblocks% ) DO CALL :DO_ONE_COLLISION

	REM Return to caller.
	goto :EOF

:DO_ONE_COLLISION

	REM Each repetition is a separate task.
	set /A "task=task+1"

	REM Each repetition uses a different target sequence.
	set /A "seq=seq+1"
	set /A "sseq=seq+100"
	set sq=%sseq:~1,2%

	REM Shorten some labels so that the filenames are not to long.
	set sz=%size:~0,1%
	set pstr=%posture:~0,2%

	REM Put all the paramters together for the compiler.
	set params=-collisions -%mass% -%posture% -delays=CollisionsSequences30.txt:%seq%

	REM Generate a script filename based on the parameters.
	set filename=GndCo%pstr%%mass%%size%%sq%.dex

	REM Generate the script to do the task.
	%COMPILER% %params%  -compile=%filename% %prep%

	REM Record the task in the protocol definition, labelled appropriately.
	echo CMD_TASK,%task%,%filename%,%task% Collisions %seq%

	REM On subsequent calls within the same block, don't redo the prep.
	set prep=

	REM Return to caller.
	goto :EOF
