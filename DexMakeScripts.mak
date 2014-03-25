# This makefile is used to make it easy to run the script generating batch file from within Virtual Studio.
# Building (F7) the DexScripts project will cause this makefile to be executed, which in turn calls 
#  the DexGenerate*.bat batch files to be executed.
# This makefile and the batch files that do the work are kept in DexSourceCode so that it gets distributed in Git.

COMPILER	= DexSimulatorApp.exe
SOURCE		= ..\DexSourceCode
DESTINATION	= ..\DexInstall

SCRIPTS		= ForceOffsets.dex FrictionTest0p5.dex FrictionTest1p5.dex InstallUprightTask.dex InstallSupineTask.dex ShowPictures.dex 
PROTOCOLS	= DexDynamicsSmall.dex DexDynamicsMedium.dex DexDynamicsLarge.dex DexSeatedSmall.dex DexSeatedMedium.dex DexSeatedLarge.dex DexSupineSmall.dex DexSupineMedium.dex DexSupineLarge.dex InstallUprightProtocol.dex InstallSupineProtocol.dex UtilitiesProtocol.dex
SESSIONS	= SessionSmallSubject.dex SessionMediumSubject.dex SessionLargeSubject.dex SessionUtilitiesOnly.dex

# The following the path to hand-edited scripts. 
STATICSCRIPTS	= ..\DexSourceCode

all: DexSimulatorApp.exe $(SCRIPTS) $(PROTOCOLS) $(SESSIONS) $(SOURCE)\DexMakeScripts.mak users.dex
	del /Q $(DESTINATION)\*.dex
	copy /Y /V *.dex $(DESTINATION) 
	echo "Last build: " %date% %time% > all
 
######################################################################################################################################

#
# Tasks 
#

### Force sensor offset compensation.
ForceOffsets.dex:	DexSimulatorApp.exe
	$(COMPILER) -offsets -compile=ForceOffsets.dex

### Coefficeiont of Friction.
FrictionTest0p5.dex:	DexSimulatorApp.exe
	$(COMPILER) -friction -pinch=0.5 -filter=10.0 -compile=FrictionTest0p5.dex
FrictionTest1p5.dex:	DexSimulatorApp.exe
	$(COMPILER) -friction -pinch=1.5 -filter=10.0 -compile=FrictionTest1p5.dex

### Configuration of DEX hardware.
InstallUprightTask.dex:	DexSimulatorApp.exe
	$(COMPILER) -install -upright -compile=InstallUpright.dex
InstallSupineTask.dex:	DexSimulatorApp.exe
	$(COMPILER) -install -supine -compile=InstallSupine.dex


######################################################################################################################################

#
# Protocols
#

### Dynamics Protocol
DexDynamicsSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamics.bat
	$(SOURCE)\DexGenerateDynamics.bat Upright Sml > DexDynamicsSmall.dex
DexDynamicsMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamics.bat
	$(SOURCE)\DexGenerateDynamics.bat Upright Med > DexDynamicsMedium.dex
DexDynamicsLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamics.bat
	$(SOURCE)\DexGenerateDynamics.bat Upright Lrg > DexDynamicsLarge.dex

### Seated Protocol
DexSeatedSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentials.bat
	$(SOURCE)\DexGenerateReferentials.bat Upright Sml > DexSeatedSmall.dex
DexSeatedMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentials.bat
	$(SOURCE)\DexGenerateReferentials.bat Upright Med > DexSeatedMedium.dex



DexSeatedLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentials.bat
	$(SOURCE)\DexGenerateReferentials.bat Upright Lrg > DexSeatedLarge.dex

### Supine Protocol

DexSupineSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentials.bat
	$(SOURCE)\DexGenerateReferentials.bat Supine Sml > DexSupineSmall.dex
DexSupineMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentials.bat
	$(SOURCE)\DexGenerateReferentials.bat Supine Med > DexSupineMedium.dex
DexSupineLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentials.bat
	$(SOURCE)\DexGenerateReferentials.bat Supine Lrg > DexSupineLarge.dex

### Utilities
UtilitiesProtocol.dex: $(STATICSCRIPTS)\UtilitiesProtocol.dex
	copy /Y $(STATICSCRIPTS)\UtilitiesProtocol.dex
InstallUprightProtocol.dex: $(STATICSCRIPTS)\InstallUprightProtocol.dex
	copy /Y $(STATICSCRIPTS)\InstallUprightProtocol.dex
InstallSupineProtocol.dex: $(STATICSCRIPTS)\InstallSupineProtocol.dex
	copy /Y $(STATICSCRIPTS)\InstallSupineProtocol.dex

######################################################################################################################################

#
# Sessions
#

SessionSmallSubject.dex:	$(STATICSCRIPTS)\SessionSmallSubject.dex
	copy /Y $(STATICSCRIPTS)\SessionSmallSubject.dex .

SessionMediumSubject.dex:	$(STATICSCRIPTS)\SessionMediumSubject.dex
	copy /Y $(STATICSCRIPTS)\SessionMediumSubject.dex .

SessionLargeSubject.dex:	$(STATICSCRIPTS)\SessionLargeSubject.dex
	copy /Y $(STATICSCRIPTS)\SessionLargeSubject.dex .

SessionUtilitiesOnly.dex:	$(STATICSCRIPTS)\SessionUtilitiesOnly.dex
	copy /Y $(STATICSCRIPTS)\SessionUtilitiesOnly.dex .


users.dex:	$(STATICSCRIPTS)\users.dex
	copy /Y $(STATICSCRIPTS)\users.dex .


# I created this task to make an easy way to flip through the different pictures.

ShowPictures.dex:	DexSimulatorApp.exe
	$(COMPILER) -pictures -compile=ShowPictures.dex

# The makefile checks if a newer version of the simulator/compiler DexSimulatorApp.exe has been generated.
# If so, it copies it here. That makes the batch files simpler, since they therefore do not
#  have to specify the path to DexSimulatorApp.exe.

$(COMPILER): ..\DexSimulatorApp\debug\DexSimulatorApp.exe
	copy ..\DexSimulatorApp\debug\DexSimulatorApp.exe .

clean:
	echo nothing > __dummy.dex
	del /Q *.dex

