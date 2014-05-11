@echo OFF

REM
REM Create an Installation task for DEX.
REM

REM Increment task counter
set /A "task=task+1"

REM Posture is passed as a parameter.
set posture=%1
set post=%posture:~0,1%

REM Pass any additional parameters as flags
set params= %2 %3 %4 %5 %6 %7 %8 %9

REM Construct a file tag from the above.
set tag=%sz%%task%Cfg%post%

REM Create the script filename.
set filename=%tag%.dex
DexSimulatorApp -install -%posture% -tag=%tag% -compile=%filename% %params% 
echo CMD_TASK,%task%,%filename%,%task% Configure %posture%


