# This makefile is used to make it easy to run the script generating batch file from within Virtual Studio.
# Building (F7) the DexScripts project will cause this makefile to be executed, which in turn calls 
#  the DexGenerate*.bat batch files to be executed.
# This makefile and the batch files that do the work are kept in DexSourceCode so that it gets distributed in Git.

COMPILER	= DexSimulatorApp.exe
SOURCE		= ..\DexSourceCode
DESTINATION	= ..\DexInstall

SCRIPTS		= ForceOffsets.dex FrictionTest0p5.dex FrictionTest1p0.dex FrictionTest2p5.dex InstallUprightTask.dex InstallSupineTask.dex ShowPictures.dex 
PROTOCOLS	= DexDynamicsFlightSmall.dex DexDynamicsFlightMedium.dex DexDynamicsFlightLarge.dex DexSeatedFlightSmall.dex DexSeatedFlightMedium.dex DexSeatedFlightLarge.dex DexSupineFlightSmall.dex DexSupineFlightMedium.dex DexSupineFlightLarge.dex DexReducedFlightSmall.dex DexReducedFlightMedium.dex DexReducedFlightLarge.dex DexDynamicsBDCSmall.dex DexDynamicsBDCMedium.dex DexDynamicsBDCLarge.dex DexSeatedFlightSmall.dex DexSeatedFlightMedium.dex DexSeatedFlightLarge.dex DexSupineFlightSmall.dex DexSupineFlightMedium.dex DexSupineFlightLarge.dex DexReducedReturnSmall.dex DexReducedReturnMedium.dex DexReducedReturnLarge.dex InstallUprightProtocol.dex InstallSupineProtocol.dex UtilitiesProtocol.dex
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
	$(COMPILER) -friction -pinch=0.5 -filter=2.0 -compile=FrictionTest0p5.dex
FrictionTest1p0.dex:	DexSimulatorApp.exe
	$(COMPILER) -friction -pinch=1.0 -filter=2.0 -compile=FrictionTest1p0.dex
FrictionTest2p5.dex:	DexSimulatorApp.exe
	$(COMPILER) -friction -pinch=2.5 -filter=2.0 -compile=FrictionTest2p5.dex

### Configuration of DEX hardware.
InstallUprightTask.dex:	DexSimulatorApp.exe
	$(COMPILER) -install -upright -compile=InstallUpright.dex
InstallSupineTask.dex:	DexSimulatorApp.exe
	$(COMPILER) -install -supine -compile=InstallSupine.dex


######################################################################################################################################

# -----------------------------------------------------------------------------------------------------------
# -- Protocols Flight (Dynamics Flight; Reference Seated Flight; Reference Supine Flight ; Flight Reduced) --
# -----------------------------------------------------------------------------------------------------------

### Dynamics Flight Protocol

DexDynamicsFlightSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamicsFlight.bat
	$(SOURCE)\DexGenerateDynamicsFlight.bat Upright Sml > DexDynamicsFlightSmall.dex
DexDynamicsFlightMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamicsFlight.bat
	$(SOURCE)\DexGenerateDynamicsFlight.bat Upright Med > DexDynamicsFlightMedium.dex
DexDynamicsFlightLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamicsFlight.bat
	$(SOURCE)\DexGenerateDynamicsFlight.bat Upright Lrg > DexDynamicsFlightLarge.dex

### Reference Seated Flight Protocol

DexSeatedFlightSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsFlight.bat
	$(SOURCE)\DexGenerateReferentialsFlight.bat Upright Sml > DexSeatedFlightSmall.dex
DexSeatedFlightMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsFlight.bat
	$(SOURCE)\DexGenerateReferentialsFlight.bat Upright Med > DexSeatedFlightMedium.dex
DexSeatedFlightLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsFlight.bat
	$(SOURCE)\DexGenerateReferentialsFlight.bat Upright Lrg > DexSeatedFlightLarge.dex

### Reference Supine Flight Protocol

DexSupineFlightSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsFlight.bat
	$(SOURCE)\DexGenerateReferentialsFlight.bat Supine Sml > DexSupineFlightSmall.dex
DexSupineFlightMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsFlight.bat
	$(SOURCE)\DexGenerateReferentialsFlight.bat Supine Med > DexSupineFlightMedium.dex
DexSupineFlightLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsFlight.bat
	$(SOURCE)\DexGenerateReferentialsFlight.bat Supine Lrg > DexSupineFlightLarge.dex

### Reduced Flight

DexReducedFlightSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateFlightReduced.bat
	$(SOURCE)\DexGenerateFlightReduced.bat Upright Sml > DexReducedFlightSmall.dex
DexReducedFlightMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateFlightReduced.bat
	$(SOURCE)\DexGenerateFlightReduced.bat Upright Med > DexReducedFlightMedium.dex
DexReducedFlightLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateFlightReduced.bat
	$(SOURCE)\DexGenerateFlightReduced.bat Upright Lrg > DexReducedFlightLarge.dex


# ------------------------------------------------------------------------------------------------
# -- Protocols BDC (Dynamics BDC; Reference Seated BDC; Reference Supine BDC ; Return Reduced) --
# ------------------------------------------------------------------------------------------------

### Dynamics BDC Protocol

DexDynamicsBDCSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamicsBDC.bat
	$(SOURCE)\DexGenerateDynamicsBDC.bat Upright Sml > DexDynamicsBDCSmall.dex
DexDynamicsBDCMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamicsBDC.bat
	$(SOURCE)\DexGenerateDynamicsBDC.bat Upright Med > DexDynamicsBDCMedium.dex
DexDynamicsBDCLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamicsBDC.bat
	$(SOURCE)\DexGenerateDynamicsBDC.bat Upright Lrg > DexDynamicsBDCLarge.dex

### Reference Seated BDC Protocol

DexSeatedBDCSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsBDC.bat
	$(SOURCE)\DexGenerateReferentialsBDC.bat Upright Sml > DexSeatedBDCSmall.dex
DexSeatedBDCMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsBDC.bat
	$(SOURCE)\DexGenerateReferentialsBDC.bat Upright Med > DexSeatedBDCMedium.dex
DexSeatedBDCLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsBDC.bat
	$(SOURCE)\DexGenerateReferentialsBDC.bat Upright Lrg > DexSeatedBDCLarge.dex

### Reference Supine BDC Protocol

DexSupineBDCSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsBDC.bat
	$(SOURCE)\DexGenerateReferentialsBDC.bat Supine Sml > DexSupineBDCSmall.dex
DexSupineBDCMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsBDC.bat
	$(SOURCE)\DexGenerateReferentialsBDC.bat Supine Med > DexSupineBDCMedium.dex
DexSupineBDCLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsBDC.bat
	$(SOURCE)\DexGenerateReferentialsBDC.bat Supine Lrg > DexSupineBDCLarge.dex

### Return Reduced

DexReducedReturnSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReturnReduced.bat
	$(SOURCE)\DexGenerateReturnReduced.bat Upright Sml > DexReducedReturnSmall.dex
DexReducedReturnMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReturnReduced.bat
	$(SOURCE)\DexGenerateReturnReduced.bat Upright Med > DexReducedReturnMedium.dex
DexReducedReturnLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReturnReduced.bat
	$(SOURCE)\DexGenerateReturnReduced.bat Upright Lrg > DexReducedReturnLarge.dex

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

