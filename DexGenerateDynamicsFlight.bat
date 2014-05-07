@echo off
SETLOCAL


REM Generate the Dynamics Flight protocol. (From a copy of the DexGenerateDynamics.bat)

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
set task=200

REM Write a header into the file.
ECHO #DEX protocol file for Dynamics Flight protocol.
ECHO #Generated by %0:  %date% %time%
ECHO #Format: "CMD_TASK", task id, task file, display name


REM ****************************************************************************

REM For common tasks we assume that the scripts have already been constructed, so we just create the appropriate lines in the procedure file.

REM ****************************************************************************

REM Standard tasks at the start of a subsession.

REM Perform the install of the equipment in the upright (seated) position.
REM Each subject should do this, even the configuration has changed, to be sure that the CODAs are aligned.
set /A "task=task+1"
echo CMD_TASK,%task%,TaskInstallUpright.dex,%task% Configure

REM Make sure that the audio is set loud enough. Also tells subject to strap in.
set /A "task=task+1"
echo CMD_TASK,%task%,TaskCheckAudio.dex,%task% Check Audio

REM The force sensor offsets are also measured and suppressed at the start for each subject.
set /A "task=task+1"
echo CMD_TASK,%task%,ForceOffsets.dex,%task% Cancel Offsets

REM ****************************************************************************

REM
REM Coefficient of Friction tests.
REM
REM !!! Need to define how many to be done and at what desired grip force.
REM !!! We also need to decide on the method.

REM Here I specify the "-sit" parameter, so that the subject receives the instructions
REM  to take a seat (or lie down) and attach the belts.
REM I do not put "-prep" because the manipulandum was placed in the retainer
REM  as part of the installation procedure.

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest0p5.dex,%task% Friction 0.5

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest1p0.dex,%task% Friction 1.0

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest2p5.dex,%task% Friction 2.5

REM ****************************************************************************

REM Now we start generating the science task scripts specific to this protocol.
REM Here we build the scripts as we go and we create the entries in the protocol file.

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

set mass=800gm
set frequency=1.00
set prep=-prep
set range=OscillationRangesNominalVertical.txt:%sz%
call :DO_ONE_OSCILLATION_TRIAL

set mass=800gm
set frequency=1.00
set prep=
set range=OscillationRangesNominalVertical.txt:%sz%
call :DO_ONE_OSCILLATION_TRIAL

set mass=400gm
set frequency=1.00
set prep=
set range=OscillationRangesNominalVertical.txt:%sz%
call :DO_ONE_OSCILLATION_TRIAL

set mass=400gm
set frequency=1.00
set prep=
set range=OscillationRangesNominalVertical.txt:%sz%
call :DO_ONE_OSCILLATION_TRIAL

set mass=600gm
set frequency=1.00
set prep=
set range=OscillationRangesNominalVertical.txt:%sz%
call :DO_ONE_OSCILLATION_TRIAL

set mass=600gm
set frequency=1.00
set prep=
set range=OscillationRangesNominalVertical.txt:%sz%
call :DO_ONE_OSCILLATION_TRIAL

set mass=600gm
set frequency=1.33
set prep=
set range=OscillationRangesNominalVertical.txt:%sz%
call :DO_ONE_OSCILLATION_TRIAL

set mass=600gm
set frequency=1.33
set prep=
set range=OscillationRangesNominalVertical.txt:%sz%
call :DO_ONE_OSCILLATION_TRIAL

set mass=600gm
set frequency=0.66
set prep=
set range=OscillationRangesNominalVertical.txt:%sz%
call :DO_ONE_OSCILLATION_TRIAL

set mass=600gm
set frequency=0.66
set prep=
set range=OscillationRangesNominalVertical.txt:%sz%
call :DO_ONE_OSCILLATION_TRIAL



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
call :DO_BLOCK_OF_TGTD

REM
REM Horizontal Direction
REM
set direction=Horizontal
call :DO_BLOCK_OF_TGTD


REM ****************************************************************************

REM
REM Coefficient of Friction tests.
REM
REM !!! Need to define how many to be done and at what desired grip force.
REM !!! We also need to decide on the method.

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest1p0prep.dex,%task% Friction 1.0

REM ****************************************************************************

REM
REM Show that protocol is finished.
REM
set /A "task=task+1"
echo CMD_TASK,%task%,TaskFinishProtocol.dex,(Done - press Back)


ENDLOCAL
goto :EOF

REM ****************************************************************************

REM Subroutines to generate a block of targeted movement trials.
REM The first initializes parameters for multiple blocks of trials.
REM It calls the second one which generates the commands for each block (task).

:DO_BLOCK_OF_TGTD

	REM Provide instructions for first block.
	REM Subroutine will disable it for subsequent blocks.
	set prep=-prep

	REM Initialize the block counter. 
	REM It gets incremented by the subroutine.
	set seq=0

	REM Procuce the required number of blocks.
	FOR /L %%s IN ( 1,1,%nblocks% ) DO CALL :DO_ONE_TARGETED

	REM Return to caller.
	goto :EOF

:DO_ONE_TARGETED

	REM Each repetition is a separate task.
	set /A "task=task+1"

	REM Each repetition uses a different target sequence.
	set /A "seq=seq+1"

	REM Shorten some labels so that the filenames are not to long.
	set dir=%direction:~0,4%
	set sz=%size:~0,1%
	set pstr=%posture:~0,2%

	REM Put all the paramters together for the compiler.
	set params=-targeted -%mass% -%posture% -%direction% -targets=TargetedTargets%direction%60.txt:%seq%%sz%  

	REM Generate a script filename based on the parameters.
	set filename=FltTg%pstr%%dir%%mass%%size%%seq%.dex

	REM Generate the script to do the task.
	%COMPILER% %params%  -compile=%filename% %prep%

	REM Record the task in the protocol definition, labelled appropriately.
	echo CMD_TASK,%task%,%filename%,%task% Targeted %dir% %seq%

	REM On subsequent calls within the same block, don't redo the prep.
	set prep=

	REM Return to caller.
	goto :EOF

REM ****************************************************************************

REM A subroutine to construct a trial of oscillations (task).
REM It assumes that the task and seq counters were initialized and
REM  that frequency, mass, range, duration and size have been set.

:DO_ONE_OSCILLATION_TRIAL

	set /A "task=task+1"
	set /A "osc_seq=osc_seq+1"
	set filename=FltOsc%pstr%%dir%%mass%%size%%osc_seq%.dex
	%COMPILER% -oscillations -%mass% -%posture% -%direction% -range=%range% -frequency=%frequency% -duration=%duration% -compile=%filename% %prep%
	echo CMD_TASK,%task%,%filename%,%task% Oscillations %osc_seq%
	goto :EOF
