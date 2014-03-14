# This makefile is used to make it easy to run the script generating batch file from within Virtual Studio.
# Building (F7) the DexScripts project will cause this makefile to be executed, which in turn calls 
#  the DexGenerateScripts.bat batch file to be executed.
# The batch files that do the work are kept in DexSourceCode so that it gets distributed in Git.

COMPILER	= DexSimulatorApp.exe
SOURCE		= ..\DexSourceCode
DESTINATION	= ..\DexInstall

SCRIPTS		= $(SOURCE)\DexMakeScripts.mak ForceOffsets.dex FrictionTest.dex InstallUpright.dex InstallSupine.dex ShowPictures.dex 
PROTOCOLS	= prot_joe.dex DexDynamicsSmall.dex
SESSIONS	= session_joe.dex

# The following the path to hand-edited scripts. 
STATICSCRIPTS	= ..\DexSourceCode

all: DexSimulatorApp.exe $(SCRIPTS) $(PROTOCOLS) $(SESSIONS) users.dex
	del /Q $(DESTINATION)\*.dex
	copy /Y /V *.dex $(DESTINATION) 
	echo "Last build: " %date% %time% > all
 
# The makefile checks if a newer version of the simulator/compiler DexSimulatorApp.exe has been generated.
# If so, it copies it here. That makes the batch files simpler, since they therefore do not
#  have to specify the path to DexSimulatorApp.exe.

$(COMPILER): ..\DexSimulatorApp\debug\DexSimulatorApp.exe
	copy ..\DexSimulatorApp\debug\DexSimulatorApp.exe .

ForceOffsets.dex:	DexSimulatorApp.exe
	$(COMPILER) -offsets -compile=ForceOffsets.dex

FrictionTest.dex:	DexSimulatorApp.exe
	$(COMPILER) -friction -compile=FrictionTest.dex

InstallUpright.dex:	DexSimulatorApp.exe
	$(COMPILER) -install -upright -compile=InstallUpright.dex

InstallSupine.dex:	DexSimulatorApp.exe
	$(COMPILER) -install -supine -compile=InstallSupine.dex

ShowPictures.dex:	DexSimulatorApp.exe
	$(COMPILER) -pictures -compile=ShowPictures.dex

DexDynamicsSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamics.bat
	$(SOURCE)\DexGenerateDynamics.bat Upright Sml > DexDynamicsSmall.dex

# Get the scripts that are edited by hand.

prot_joe.dex:	$(STATICSCRIPTS)\prot_joe.dex
	copy /Y $(STATICSCRIPTS)\prot_joe.dex

session_joe.dex:	$(STATICSCRIPTS)\session_joe.dex
	copy /Y $(STATICSCRIPTS)\session_joe.dex

users.dex:	$(STATICSCRIPTS)\users.dex
	copy /Y $(STATICSCRIPTS)\users.dex

clean:
	echo nothing > __dummy.dex
	del /Q *.dex

