@echo off
SETLOCAL

REM
REM  DexGeneratePFProtocol.bat
REM

REM A batch file used to create protocols for each subject in parabolic flight.

REM First parameter is the subject number.
set ID=%1

set SOURCE=..\DexSourceCode

echo # DEX session file for parabolic flight subject %ID% 
echo # Format: CMD_PROTOCOL,id,filename,menu


call %SOURCE%\DexGenerateParabolicFlight.bat Upright %ID% > %ID%Upright.dex
echo CMD_PROTOCOL,180,%ID%Upright.dex,PF Upright

call %SOURCE%\DexGenerateParabolicFlight.bat Supine %ID% > %ID%Supine.dex
echo CMD_PROTOCOL,181,%ID%Supine.dex,PF Supine


echo CMD_PROTOCOL,101,ProtocolSensorTests.dex,Test Sensors
echo CMD_PROTOCOL,100,ProtocolUtilities.dex,Utilities

