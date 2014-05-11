@echo OFF

REM
REM Create a Friction Test task for DEX.
REM

REM The grip force is passed as a parameter.
set force=%1

REM Increment task counter
set /A "task=task+1"

REM Construct a file tag from the above.
set tag=%sz%%task%FRIC

REM Pass any additional parameters as flags
set params=%2 %3 %4 %5 %6 %7 %8 %9

REM Create the script filename.
set filename=%tag%.dex
DexSimulatorApp -friction -filter=2.0 -tag=%tag% -pinch=%force% -compile=%filename% %params% 
echo CMD_TASK,%task%,%filename%,%task% Friction %force%


