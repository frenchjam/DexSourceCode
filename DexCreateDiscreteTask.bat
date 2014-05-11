@echo OFF

REM
REM Create an Discrete Test task for DEX.
REM

REM Assumes that task, mass, posture, direction, range, etc. are set already.
set /A "task=task+1"
set /A "dsc_seq=dsc_seq+1"
set dir=%direction:~0,4%

REM Construct a file tag.
set tag=%sz%%task%

REM Create a filename.
set filename=%tag%Dsc%dir%%mass%%dsc_seq%.dex
%COMPILER% -discrete -%mass% -%posture% -%direction% -%eyes% -range=%range% -delays=DiscreteDelaySequences30.txt:%dsc_seq% -tag=%tag% -compile=%filename%  %*
echo CMD_TASK,%task%,%filename%,%task% Discrete %dsc_seq%
