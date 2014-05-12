# This makefile is used to make it easy to run the script generating batch file from within Virtual Studio.
# Building (F7) the DexScripts project will cause this makefile to be executed, which in turn calls 
#  the DexGenerate*.bat batch files to be executed.
# This makefile and the batch files that do the work are kept in DexSourceCode so that it gets distributed in Git.

COMPILER	= DexSimulatorApp.exe
SOURCE		= ..\DexSourceCode
DESTINATION	= ..\GripReleases

SCRIPTS		= TaskCheckAudio.dex TaskFinishProtocol.dex
PROTOCOLS	= ProtocolInstallUpright.dex ProtocolInstallSupine.dex ProtocolUtilities.dex

HELPERS		= $(SOURCE)\DexCreateFrictionTask.bat $(SOURCE)\DexCreateInstallTask.bat $(SOURCE)\DexCreateOffsetTask.bat $(SOURCE)\DexCreateOscillationTask.bat $(SOURCE)\DexCreateDiscreteTask.bat $(SOURCE)\DexCreateTargetedTasks.bat $(SOURCE)\DexCreateCollisionTasks.bat

# The following the path to hand-edited scripts. 
STATICSCRIPTS	= ..\DexSourceCode
# The following the path to picture files. 
PICTURES		= ..\DexPictures

LINT	= ..\DexLint\debug\DexLint.exe
TAR		=	"C:\Program Files\GnuWin32\bin\tar.exe"
MD5TREE	=	..\bin\MD5Tree.exe

default: release

all: GripFlightScripts.tar

######################################################################################################################################

GripFlightScripts.tar: $(SOURCE)\DexMakeScripts.mak DexSimulatorApp.exe users_flight.dex
	copy /Y /V users_flight.dex users.dex
	$(LINT) -noquery -pictures=$(PICTURES) users.dex -sbatch=CreateFlightScriptsTar.bat -pbatch=CreateFlightPicturesTar.bat -log=GripFlightLint.log
	CreateFlightScriptsTar.bat GripFlightScripts.tar

# The following would be a better way, but I can't get tar to work like it should.
#	$(TAR) --create --verbose --files-from=DexLintScripts.log --file=DexFlightScripts.tar 
#	$(TAR) --create --verbose --directory=$(PICTURES) --files-from=DexLintPictures.log --file=DexFlightScripts.tar 

GripFlightPictures.tar: GripFlightScripts.tar
	CreateFlightPicturesTar.bat GripFlightPictures.tar

GripFlight.md5: GripFlightPictures.tar GripFlightScripts.tar
	$(MD5TREE)  GripFlightPictures.tar GripFlightScripts.tar > GripFlight.md5

release: GripFlightScripts.tar GripFlightPictures.tar GripFlight.md5 
	$(SOURCE)\DexReleaseScripts.bat GripFlight
	 
######################################################################################################################################

#
# Users
#

users_flight.dex:	$(SOURCE)\DexGenerateSubjects.bat SessionSmallSubjectFlight.dex SessionMediumSubjectFlight.dex SessionLargeSubjectFlight.dex SessionU.dex
	$(SOURCE)\DexGenerateSubjects.bat Flight users_flight.dex

######################################################################################################################################

#
# Sessions
#

SessionSmallSubjectFlight.dex:	$(STATICSCRIPTS)\SessionSmallSubjectFlight.dex $(PROTOCOLS) DexDynamicsFlightSmall.dex DexSeatedFlightSmall.dex DexSupineFlightSmall.dex DexReducedFlightSmall.dex
	copy /Y $(STATICSCRIPTS)\SessionSmallSubjectFlight.dex .\$@

SessionMediumSubjectFlight.dex:	$(STATICSCRIPTS)\SessionMediumSubjectFlight.dex $(PROTOCOLS) DexDynamicsFlightMedium.dex DexSeatedFlightMedium.dex DexSupineFlightMedium.dex DexReducedFlightMedium.dex
	copy /Y $(STATICSCRIPTS)\SessionMediumSubjectFlight.dex .\$@

SessionLargeSubjectFlight.dex:	$(STATICSCRIPTS)\SessionLargeSubjectFlight.dex $(PROTOCOLS) DexDynamicsFlightLarge.dex DexSeatedFlightLarge.dex DexSupineFlightLarge.dex DexReducedFlightLarge.dex
	copy /Y $(STATICSCRIPTS)\SessionLargeSubjectFlight.dex .\$@

SessionSmallSubjectBDC.dex:	$(STATICSCRIPTS)\SessionSmallSubjectBDC.dex $(PROTOCOLS) DexDynamicsBDCSmall.dex DexSeatedBDCSmall.dex DexSupineBDCSmall.dex DexReducedBDCSmall.dex
	copy /Y $(STATICSCRIPTS)\SessionSmallSubjectBDC.dex .\$@

SessionMediumSubjectBDC.dex:	$(STATICSCRIPTS)\SessionMediumSubjectBDC.dex $(PROTOCOLS) DexDynamicsBDCMedium.dex DexSeatedBDCMedium.dex DexSupineBDCMedium.dex DexReducedBDCMedium.dex
	copy /Y $(STATICSCRIPTS)\SessionMediumSubjectBDC.dex .\$@

SessionLargeSubjectBDC.dex:	$(STATICSCRIPTS)\SessionLargeSubjectBDC.dex $(PROTOCOLS) DexDynamicsBDCLarge.dex DexSeatedBDCLarge.dex DexSupineBDCLarge.dex DexReducedBDCLarge.dex
	copy /Y $(STATICSCRIPTS)\SessionLargeSubjectBDC.dex .\$@

SessionU.dex:	$(STATICSCRIPTS)\SessionUtilitiesOnly.dex $(PROTOCOLS) 
	copy /Y $(STATICSCRIPTS)\SessionUtilitiesOnly.dex .\$@


######################################################################################################################################

# -----------------------------------------------------------------------------------------------------------
# -- Protocols Flight (Dynamics Flight; Reference Seated Flight; Reference Supine Flight ; Flight Reduced) --
# -----------------------------------------------------------------------------------------------------------

### Dynamics Flight Protocol

DexDynamicsFlightSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamicsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsFlight.bat Upright Sml > $@
DexDynamicsFlightMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamicsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsFlight.bat Upright Med > $@
DexDynamicsFlightLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamicsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsFlight.bat Upright Lrg > $@

### Reference Seated Flight Protocol

DexSeatedFlightSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsFlight.bat Upright Sml > $@
DexSeatedFlightMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsFlight.bat Upright Med > $@
DexSeatedFlightLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsFlight.bat Upright Lrg > $@

### Reference Supine Flight Protocol

DexSupineFlightSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsFlight.bat Supine Sml > $@
DexSupineFlightMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsFlight.bat Supine Med > $@
DexSupineFlightLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsFlight.bat Supine Lrg > $@

### Reduced Flight

DexReducedFlightSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateFlightReduced.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateFlightReduced.bat Upright Sml > $@
DexReducedFlightMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateFlightReduced.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateFlightReduced.bat Upright Med > $@
DexReducedFlightLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateFlightReduced.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateFlightReduced.bat Upright Lrg > $@


# ------------------------------------------------------------------------------------------------
# -- Protocols BDC (Dynamics BDC; Reference Seated BDC; Reference Supine BDC ; Return Reduced) --
# ------------------------------------------------------------------------------------------------

### Dynamics BDC Protocol

DexDynamicsBDCSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamicsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsBDC.bat Upright Sml > $@
DexDynamicsBDCMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamicsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsBDC.bat Upright Med > $@
DexDynamicsBDCLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDynamicsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsBDC.bat Upright Lrg > $@

### Reference Seated BDC Protocol

DexSeatedBDCSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsBDC.bat Upright Sml > $@
DexSeatedBDCMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsBDC.bat Upright Med > $@
DexSeatedBDCLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsBDC.bat Upright Lrg > $@

### Reference Supine BDC Protocol

DexSupineBDCSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsBDC.bat Supine Sml > $@
DexSupineBDCMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsBDC.bat Supine Med > $@
DexSupineBDCLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReferentialsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsBDC.bat Supine Lrg > $@

### Return Reduced

DexReducedBDCSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReducedBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReturnReduced.bat Upright Sml > $@
DexReducedBDCMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReducedBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReturnReduced.bat Upright Med > $@
DexReducedBDCLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReducedBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReturnReduced.bat Upright Lrg > $@

### Utilities

ProtocolUtilities.dex: $(STATICSCRIPTS)\ProtocolUtilities.dex calibrate.dex task_align.dex task_nullify.dex task_shutdown.dex
	copy /Y $(STATICSCRIPTS)\ProtocolUtilities.dex
ProtocolInstallUpright.dex: $(STATICSCRIPTS)\ProtocolInstallUpright.dex ForceOffsets.dex TaskInstallUpright.dex 
	copy /Y $(STATICSCRIPTS)\ProtocolInstallUpright.dex
ProtocolInstallSupine.dex: $(STATICSCRIPTS)\ProtocolInstallSupine.dex ForceOffsets.dex TaskInstallSupine.dex 
	copy /Y $(STATICSCRIPTS)\ProtocolInstallSupine.dex

######################################################################################################################################

#
# Tasks 
#

### Configuration of DEX hardware.

TaskInstallUpright.dex:	DexSimulatorApp.exe
	$(COMPILER) -install -upright -compile=$@
TaskInstallSupine.dex:	DexSimulatorApp.exe
	$(COMPILER) -install -supine -compile=$@
TaskFinishProtocol.dex: DexSimulatorApp.exe
	$(COMPILER) -finish -compile=$@
TaskCheckAudio.dex: DexSimulatorApp.exe
	$(COMPILER) -audio -sit -compile=$@
ForceOffsets.dex: DexSimulatorApp.exe
	$(COMPILER) -offsets -compile=$@

### Utilities

calibrate.dex: $(STATICSCRIPTS)\calibrate.dex
	copy /Y $(STATICSCRIPTS)\calibrate.dex .
task_align.dex: $(STATICSCRIPTS)\task_align.dex
	copy /Y $(STATICSCRIPTS)\task_align.dex .
task_nullify.dex: $(STATICSCRIPTS)\task_nullify.dex
	copy /Y $(STATICSCRIPTS)\task_nullify.dex .
task_shutdown.dex: $(STATICSCRIPTS)\task_shutdown.dex
	copy /Y $(STATICSCRIPTS)\task_shutdown.dex .


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

