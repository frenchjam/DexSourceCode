@echo off
REM Scripts related to the targeted movements protocols.

REM This script also allows one to select -upright or -supine with the first parameter.
set posture=%1
set pstr=%posture:~0,2%

REM This script takes an input parameter, which can be sml, med, lrg. It adds this as a modifier to the target file list
REM so that the targets can be tuned to different size subject. It also inserts this string into the script file name
set size=%2
set sz=%size:~0,1%

REM Alias to the compiler.
set COMPILER=DexSimulatorApp.exe

REM Task Counter
set task=200

ECHO "#DEX protocol file for Dynamics protocol."
ECHO "#Format: "CMD_TASK", task id, task file, display name"

REM Standard tasks at the start of a subsession.
set /A "task=task+1"
echo CMD_TASK,%task%,InstallUpright.dex,%task% Install

set /A "task=task+1"
echo CMD_TASK,%task%,ForceOffsets.dex,%task% Cancel Offsets

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest0p5.dex,%task% Friction 0.5

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest1p5.dex,%task% Friction 1.5

REM Targeted movements in the vertical direction.
set mass=400gm
set params=-targeted -%mass% -%posture% -vertical
set seq=0

set /A "task=task+1"
set /A "seq=seq+1"
set filename=Tg%pstr%Vert%mass%%size%%seq%.dex
%COMPILER% %params% -targets=TargetedTargetsVertical.txt:%seq%%sz% -compile=%filename% -prep
echo CMD_TASK,%task%,%filename%,%task% Targeted Vert %seq%

set /A "task=task+1"
set /A "seq=seq+1"
set filename=Tg%pstr%Vert%mass%%size%%seq%.dex
%COMPILER% %params% -targets=TargetedTargetsVertical.txt:%seq%%sz% -compile=%filename% 
echo CMD_TASK,%task%,%filename%,%task% Targeted Vert %seq%

REM Targeted movements in the vertical direction.
set mass=400gm
set params=-targeted -%mass% -%posture% -horizontal
set seq=0

set /A "task=task+1"
set /A "seq=seq+1"
set filename=Tg%pstr%Hori%mass%%size%%seq%.dex
%COMPILER% %params% -targets=TargetedTargetsHorizontal.txt:%seq%%sz% -compile=%filename% -prep
echo CMD_TASK,%task%,%filename%,%task% Targeted Hori %seq%

set /A "task=task+1"
set /A "seq=seq+1"
set filename=Tg%pstr%Hori%mass%%size%%seq%.dex
%COMPILER% %params% -targets=TargetedTargetsHorizontal.txt:%seq%%sz% -compile=%filename% 
echo CMD_TASK,%task%,%filename%,%task% Targeted Hori %seq%

set /A "task=task+1"
echo CMD_TASK,%task%,FrictionTest.dex,%task% Friction Test
