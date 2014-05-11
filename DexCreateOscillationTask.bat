@echo OFF

REM
REM Create an Oscillations Test task for DEX.
REM

REM Assumes that task, mass, posture, direction, range, frequency, duration, etc. are set already.
set /A "task=task+1"
set /A "osc_seq=osc_seq+1"
set os=%osc_seq:~1,2%

REM Construct a file tag.
set dir=%direction:~0,1%
set post=%posture:~0,1%
set ms=%mass:~0,1%
set tag=%sz%%task%O%post%%dir%%ms%

REM Create a filename.
REM set filename=%tag%Osc%dir%%mass%%os%.dex
set filename=%tag%%os%.dex
%COMPILER% -oscillations -%mass% -%posture% -%direction% -range=%range% -frequency=%frequency% -duration=%duration% -tag=%tag% -compile=%filename% %*
echo CMD_TASK,%task%,%filename%,%task% Oscillations %os%

