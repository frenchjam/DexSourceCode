@echo off
SETLOCAL


REM Generate the sensor test protocol. 

REM Alias to the compiler.
set COMPILER=DexSimulatorApp.exe
set SOURCE=..\DexSourceCode

REM Task Counter
REM Here I am adopting a strategy by which all the protocols of the same type but for different size subjects
REM  (small, medium and large) will use the same ID number for each task, but that the different protocols will 
REM  start numbering from different values, i.e. Dynamics = 200, Upright = 300, Supine = 400, Reduced = 500
set task=970

REM Write a header into the file.
ECHO #DEX protocol file for sensor tests.
ECHO #Generated by %0:  %date% %time%
ECHO #Format: "CMD_TASK", task id, task file, display name

REM Perform the install of the equipment..
call %SOURCE%\DexCreateInstallTask.bat Upright

REM The force sensor offsets are also measured and suppressed at the start for each subject.
REM This needs to be done in preparation for the friction test.
call %SOURCE%\DexCreateOffsetTask.bat -deploy -stow

REM ****************************************************************************

REM
REM Calibration movements.
REM

set duration=10.0
set frequency=1.0

set /A "task=task+1"
set tag=X%task%USLW
set filename=%tag%.dex
%COMPILER% -sensors -prep -vertical -frequency=%frequency% -duration=%duration% -tag=%tag% -compile=%filename% 
echo CMD_TASK,%task%,%filename%,%task% Up-Down Slow

set /A "task=task+1"
set tag=X%task%DSLW
set filename=%tag%.dex
%COMPILER% -sensors -depth -frequency=%frequency% -duration=%duration% -tag=%tag% -compile=%filename% 
echo CMD_TASK,%task%,%filename%,%task% Front-Back Slow

set /A "task=task+1"
set tag=X%task%SSLW
set filename=%tag%.dex
%COMPILER% -sensors -sideways -frequency=%frequency% -duration=%duration% -tag=%tag% -compile=%filename% 
echo CMD_TASK,%task%,%filename%,%task% Left-Right Slow

set duration=10.0
set frequency=1.33

set /A "task=task+1"
set tag=X%task%VFST
set filename=%tag%.dex
%COMPILER% -sensors -vertical -frequency=%frequency% -duration=%duration% -tag=%tag% -compile=%filename% 
echo CMD_TASK,%task%,%filename%,%task% Up-Down Fast

set /A "task=task+1"
set tag=X%task%DFST
set filename=%tag%.dex
%COMPILER% -sensors -depth -frequency=%frequency% -duration=%duration% -tag=%tag% -compile=%filename% 
echo CMD_TASK,%task%,%filename%,%task% Front-Back Fast

set /A "task=task+1"
set tag=X%task%SFST
set filename=%tag%.dex
%COMPILER% -sensors -sideways -frequency=%frequency% -duration=%duration% -tag=%tag% -compile=%filename% 
echo CMD_TASK,%task%,%filename%,%task% Left-Right Fast

REM ****************************************************************************

REM
REM Show that protocol is finished.
REM
set /A "task=task+1"
echo CMD_TASK,%task%,TaskFinishProtocol.dex,(Done - press Back)

ENDLOCAL
goto :EOF
