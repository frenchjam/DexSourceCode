@echo off
SETLOCAL
REM Scripts related to the Dynamics protocol.

REM This script allows one to select -upright or -supine with the first parameter.
set posture=%1

REM This script takes an input parameter, which can be sml, med, lrg. It adds this as a modifier to the target file list
REM so that the targets can be tuned to different size subject. It also inserts this string into the script file name
set size=%2

REM Alias to the compiler.
set COMPILER=DexSimulatorApp.exe

REM Task Counter
REM Here I am adopting a strategy by which all the protocols of the same type but for different size subjects
REM  (small, medium and large) will use the same ID number for each task, but that the different protocols will 
REM  start numbering from different values, i.e. Dynamics = 200, Upright = 300, Supine = 400.
set task=200

REM Write a header into the file.
ECHO "#DEX protocol file for Dynamics protocol."
ECHO "#Format: "CMD_TASK", task id, task file, display name"
ECHO "#Generated: " %date% %time%

REM Standard tasks at the start of a subsession.
REM We assume that the scripts have already been constructed.
set /A "task=task+1"
echo CMD_TASK,%task%,InstallUpright.dex,%task% Install

set /A "task=task+1"
echo CMD_TASK,%task%,ForceOffsets.dex,%task% Cancel Offsets

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest0p5.dex,%task% Friction 0.5

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest1p5.dex,%task% Friction 1.5

REM Now we start generating the science tasks specific to this protocol.
REM Here we build the scripts as we go.

REM
REM Targeted Movements
REM

set mass=400gm
set nblocks=5

REM
REM Vertical Direction
REM
set direction=Vertical
call :DOBLOCKTGTD

REM
REM Horizontal Direction
REM
set direction=Horizontal
call :DOBLOCKTGTD


REM Closeout tasks. 
REM As for the intial tasks, we assume that the scripts exist already.
set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest.dex,%task% Friction Test

ENDLOCAL
goto :EOF

REM ****************************************************************************

REM Subroutines to generate a block of targeted movement trials.

:DOBLOCKTGTD

	REM Provide instructions for first block.
	REM Subroutine will disable it for subsequent blocks.
	set prep=-prep

	REM Initialize the block counter. 
	REM It gets incremented by the subroutine.
	set seq=0

	REM Procuce the required number of blocks.
	FOR /L %%s IN ( 1,1,%nblocks% ) DO CALL :DOONETGTD
	goto :EOF

:DOONETGTD
	REM Each repetition is a separate task.
	set /A "task=task+1"
	REM Each repetition uses a different target sequence.
	set /A "seq=seq+1"
	REM Shorten some labels so that the filenames are not to long.
	set dir=%direction:~0,4%
	set sz=%size:~0,1%
	set pstr=%posture:~0,2%
	REM Put all the paramters together for the compiler.
	set params=-targeted %prep% -%mass% -%posture% -%direction% -targets=TargetedTargets%direction%.txt:%seq%%sz%
	REM Generate a script filename based on the parameters.
	set filename=Tg%pstr%%dir%%mass%%size%%seq%.dex
	REM Generate the script to do the task.
	%COMPILER% %params%  -compile=%filename% 
	REM Record the task in the protocol definition, labelled appropriately.
	echo CMD_TASK,%task%,%filename%,%task% Targeted %dir% %seq%
	REM On subsequent calls, don't redo the prep.
	set prep=""
	goto :EOF
