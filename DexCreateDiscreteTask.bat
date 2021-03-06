@echo OFF

REM
REM Create an Discrete Test task for DEX.
REM

REM Assumes that task, mass, posture, direction, range, etc. are set already.
set /A "task=task+1"
set /A "dsc_seq=dsc_seq+1"

REM Construct a file tag.
set dir=%direction:~0,1%
set post=%posture:~0,1%
set ms=%mass:~0,1%
set ey=%eyes:~0,2%
set tag=%sz%%task%D%post%%dir%%ms%

REM Create a filename.
set filename=%tag%%ey%.dex
%COMPILER% -discrete -%mass% -%posture% -%direction% -%eyes% -range=%range% -delays=%delays%:%dsc_seq% -tag=%tag% -compile=%filename%  %*
echo CMD_TASK,%task%,%filename%,%task% Discrete %dsc_seq% %dir%%eyes:~0,1%
