# This makefile is used to make it easy to run the script generating batch file from within Virtual Studio.
# Building (F7) the DexScripts project will cause this makefile to be executed, which in turn calls 
#  the DexGenerate*.bat batch files to be executed.
# This makefile and the batch files that do the work are kept in DexSourceCode so that it gets distributed in Git.

COMPILER	= DexSimulatorApp.exe
SOURCE		= ..\DexSourceCode
DESTINATION	= ..\DexInstall

SCRIPTS		= calibrate.dex ForceOffsets.dex FrictionTest0p5.dex FrictionTest1p0.dex FrictionTest2p5.dex FrictionTest0p5prep.dex FrictionTest1p0prep.dex FrictionTest2p5prep.dex FrictionTest0p5sit.dex FrictionTest1p0sit.dex FrictionTest2p5sit.dex InstallUprightTask.dex InstallSupineTask.dex ShowPictures.dex 
FLIGHT		= DexDynamicsFlightSmall.dex DexDynamicsFlightMedium.dex DexDynamicsFlightLarge.dex DexSeatedFlightSmall.dex DexSeatedFlightMedium.dex DexSeatedFlightLarge.dex DexSupineFlightSmall.dex DexSupineFlightMedium.dex DexSupineFlightLarge.dex DexReducedFlightSmall.dex DexReducedFlightMedium.dex DexReducedFlightLarge.dex SessionSmallSubjectFlight.dex SessionMediumSubjectFlight.dex SessionLargeSubjectFlight.dex  
GROUND		= DexDynamicsBDCSmall.dex DexDynamicsBDCMedium.dex DexDynamicsBDCLarge.dex DexSeatedFlightSmall.dex DexSeatedFlightMedium.dex DexSeatedFlightLarge.dex DexSupineFlightSmall.dex DexSupineFlightMedium.dex DexSupineFlightLarge.dex DexReducedReturnSmall.dex DexReducedReturnMedium.dex DexReducedReturnLarge.dex          SessionSmallSubjectBDC.dex SessionMediumSubjectBDC.dex SessionLargeSubjectBDC.dex 
COMMON		= ProtocolInstallUpright.dex ProtocolInstallSupine.dex ProtocolUtilities.dex SessionUtilitiesOnly.dex

# The following the path to hand-edited scripts. 
STATICSCRIPTS	= ..\DexSourceCode

TAR		=	"C:\Program Files\GnuWin32\bin\tar.exe"
MD5TREE	=	..\bin\MD5Tree.exe

all: DexFlightScripts.tar DexGroundScripts.tar

DexFlightScripts.tar: DexSimulatorApp.exe $(SCRIPTS) $(FLIGHT) $(COMMON) $(SOURCE)\DexMakeScripts.mak users_flight.dex
	copy /Y /V users_flight.dex users.dex
	$(MD5TREE) $(SCRIPTS) $(FLIGHT) $(COMMON) Flt*.dex users.dex > DexFlightScripts.md5
	$(TAR) --create --verbose --file=DexFlightScripts.tar $(SCRIPTS) $(FLIGHT) $(COMMON) Flt*.dex users.dex DexFlightScripts.md5
	copy /Y /V DexFlightScripts.tar "$(DESTINATION)\DexFlightScripts (%date:~10,4%.%date:~7,2%.%date:~4,2% %time:~0,2%H%time:~3,2%).tar"

DexGroundScripts.tar: DexSimulatorApp.exe $(SCRIPTS) $(GROUND) $(COMMON) $(SOURCE)\DexMakeScripts.mak users_ground.dex
	copy /Y /V users_ground.dex users.dex
	$(MD5TREE) $(SCRIPTS) $(GROUND) $(COMMON) Gnd*.dex users.dex > DexGroundScripts.md5
	$(TAR) --create --verbose --file=DexGroundScripts.tar $(SCRIPTS) $(GROUND) $(COMMON) Gnd*.dex users.dex DexGroundScripts.md5
	copy /Y /V DexGroundScripts.tar "$(DESTINATION)\DexGroundScripts (%date:~10,4%.%date:~7,2%.%date:~4,2% %time:~0,2%H%time:~3,2%).tar"

 
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

FrictionTest0p5prep.dex:	DexSimulatorApp.exe
	$(COMPILER) -friction -pinch=0.5 -filter=2.0 -compile=FrictionTest0p5prep.dex -prep
FrictionTest1p0prep.dex:	DexSimulatorApp.exe
	$(COMPILER) -friction -pinch=1.0 -filter=2.0 -compile=FrictionTest1p0prep.dex -prep
FrictionTest2p5prep.dex:	DexSimulatorApp.exe
	$(COMPILER) -friction -pinch=2.5 -filter=2.0 -compile=FrictionTest2p5prep.dex -prep

FrictionTest0p5sit.dex:	DexSimulatorApp.exe
	$(COMPILER) -friction -pinch=0.5 -filter=2.0 -compile=FrictionTest0p5sit.dex -sit
FrictionTest1p0sit.dex:	DexSimulatorApp.exe
	$(COMPILER) -friction -pinch=1.0 -filter=2.0 -compile=FrictionTest1p0sit.dex -sit
FrictionTest2p5sit.dex:	DexSimulatorApp.exe
	$(COMPILER) -friction -pinch=2.5 -filter=2.0 -compile=FrictionTest2p5sit.dex -sit

### Configuration of DEX hardware.
InstallUprightTask.dex:	DexSimulatorApp.exe
	$(COMPILER) -install -upright -compile=InstallUprightTask.dex
InstallSupineTask.dex:	DexSimulatorApp.exe
	$(COMPILER) -install -supine -compile=InstallSupineTask.dex


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

ProtocolUtilities.dex: $(STATICSCRIPTS)\ProtocolUtilities.dex
	copy /Y $(STATICSCRIPTS)\ProtocolUtilities.dex
ProtocolInstallUpright.dex: $(STATICSCRIPTS)\ProtocolInstallUpright.dex
	copy /Y $(STATICSCRIPTS)\ProtocolInstallUpright.dex
ProtocolInstallSupine.dex: $(STATICSCRIPTS)\ProtocolInstallSupine.dex
	copy /Y $(STATICSCRIPTS)\ProtocolInstallSupine.dex

######################################################################################################################################

#
# Sessions
#

SessionSmallSubjectFlight.dex:	$(STATICSCRIPTS)\SessionSmallSubjectFlight.dex
	copy /Y $(STATICSCRIPTS)\SessionSmallSubjectFlight.dex .

SessionMediumSubjectFlight.dex:	$(STATICSCRIPTS)\SessionMediumSubjectFlight.dex
	copy /Y $(STATICSCRIPTS)\SessionMediumSubjectFlight.dex .

SessionLargeSubjectFlight.dex:	$(STATICSCRIPTS)\SessionLargeSubjectFlight.dex
	copy /Y $(STATICSCRIPTS)\SessionLargeSubjectFlight.dex .

SessionSmallSubjectBDC.dex:	$(STATICSCRIPTS)\SessionSmallSubjectBDC.dex
	copy /Y $(STATICSCRIPTS)\SessionSmallSubjectBDC.dex .

SessionMediumSubjectBDC.dex:	$(STATICSCRIPTS)\SessionMediumSubjectBDC.dex
	copy /Y $(STATICSCRIPTS)\SessionMediumSubjectBDC.dex .

SessionLargeSubjectBDC.dex:	$(STATICSCRIPTS)\SessionLargeSubjectBDC.dex
	copy /Y $(STATICSCRIPTS)\SessionLargeSubjectBDC.dex .

SessionUtilitiesOnly.dex:	$(STATICSCRIPTS)\SessionUtilitiesOnly.dex
	copy /Y $(STATICSCRIPTS)\SessionUtilitiesOnly.dex .


######################################################################################################################################

#
# Users
#

users_flight.dex:	$(STATICSCRIPTS)\users_flight.dex
	copy /Y $(STATICSCRIPTS)\users_flight.dex .

users_ground.dex:	$(STATICSCRIPTS)\users_ground.dex
	copy /Y $(STATICSCRIPTS)\users_ground.dex .

######################################################################################################################################

#
# Utilities
#

calibrate.dex: $(STATICSCRIPTS)\calibrate.dex
	copy /Y $(STATICSCRIPTS)\calibrate.dex .

######################################################################################################################################

#
# Pictures Utility
#

# I created this task to make an easy way to flip through the different pictures.

ShowPictures.dex:	DexSimulatorApp.exe
	$(COMPILER) -pictures -compile=ShowPictures.dex

######################################################################################################################################

#
# Compiler / Simulator
#

# The makefile checks if a newer version of the simulator/compiler DexSimulatorApp.exe has been generated.
# If so, it copies it here. That makes the batch files simpler, since they therefore do not
#  have to specify the path to DexSimulatorApp.exe.
# If the compiler changed, we delete all the existing .dex files so that they all get rebuilt, just in case.

$(COMPILER): ..\DexSimulatorApp\debug\DexSimulatorApp.exe
	echo nothing > __dummy.dex
	del /Q *.dex
	copy ..\DexSimulatorApp\debug\DexSimulatorApp.exe .

clean:
	echo nothing > __dummy.dex
	del /Q *.dex

