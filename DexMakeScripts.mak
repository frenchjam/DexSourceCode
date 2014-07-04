# This makefile is used to make it easy to run the script generating batch file from within Virtual Studio.
# Building (F7) the DexScripts project will cause this makefile to be executed, which in turn calls 
#  the DexGenerate*.bat batch files to be executed.
# This makefile and the batch files that do the work are kept in DexSourceCode so that it gets distributed in Git.

# Uncomment one of these options or make up your own.
# F7 will build the first 'default:' specification that it finds.
# Careful - 'default' there can be no spaces before 'default:'
#default:	clean					# Pick this if you want to clear out all the .dex files as rebuild everything.
#default:	release_flight		# Generate a flight release.
default:	release_ground		# Generate a ground release.

# This is here to catch an error where you did not pick a default build.
default_default:
	@echo **********************************************************************************************************************************
	@echo You need to pick the build that you want.
	@echo See beginning of DexMakeScripts.mak.
	@echo **********************************************************************************************************************************
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
PROOFS			= ..\GripScreenShots

LINT	= ..\DexLint\debug\DexLint.exe
TAR		=	"C:\Program Files\GnuWin32\bin\tar.exe"
MD5TREE	=	..\bin\MD5Tree.exe

TOUCH = copy /b $@ +,,

all: GripFlightScripts.tar GripFlightPictures.tar GripFlightProofs.tar GripFlight.md5
almost_all: GripFlightScripts.tar GripFlightPictures.tar GripFlightMessageList.txt 

######################################################################################################################################

GripFlightScripts.tar: $(LINT) users_flight.dex _check_messages.dex
	copy /Y /V users_flight.dex users.dex
	$(LINT) -noquery -pictures=$(PICTURES) users.dex -sbatch=CreateFlightScriptsTar.bat -log=GripFlightLintScripts.log
	CreateFlightScriptsTar.bat GripFlightScripts.tar

GripFlightPictures.tar:  $(LINT) users_flight.dex _check_messages.dex
	copy /Y /V users_flight.dex users.dex
	$(LINT) -noquery -pictures=$(PICTURES) users.dex -pbatch=CreateFlightPicturesTar.bat -log=GripFlightLintPictures.log
	CreateFlightPicturesTar.bat GripFlightPictures.tar

GripFlightMessageList.txt: $(LINT) GripFlightScripts.tar _check_messages.dex
	copy /Y /V users_flight.dex users.dex
	$(LINT) -noquery -pictures=$(PICTURES) users.dex -messages=GripFlightMessageList.txt -log=GripFlightLintMessages.log
	copy /Y /V GripFlightMessageList.txt $(PICTURES)

GripFlightProofs: $(LINT) GripFlightScripts.tar
	copy /Y /V users_flight.dex users.dex
	echo this > $(PROOFS)\deletethis.txt
	del /Q $(PROOFS)\*.*
	$(LINT) -noquery -pictures=$(PICTURES) users.dex -messages=GripFlightMessageList.txt -proofs=$(PROOFS) -log=GripFlightLintProofs.log
	copy /Y /V GripFlightMessageList.txt $(PICTURES)
	echo %date% %time% > GripFlightProofs

GripFlightProofs.tar: GripFlightProofs
	$(TAR) --create --verbose --file=GripFlightProofs.tar --directory=$(PROOFS) *.bmp

GripFlight.md5: GripFlightPictures.tar GripFlightScripts.tar
	$(MD5TREE)  GripFlightPictures.tar GripFlightScripts.tar > GripFlight.md5

release_flight: GripFlightScripts.tar GripFlightPictures.tar GripFlight.md5 
	$(SOURCE)\DexReleaseScripts.bat GripFlight
	echo %date% %time% > $@
	 
######################################################################################################################################

GripGroundScripts.tar: $(LINT) users_ground.dex _check_messages.dex
	copy /Y /V users_ground.dex users.dex
	$(LINT) -noquery -pictures=$(PICTURES) users.dex -sbatch=CreateGroundScriptsTar.bat -log=GripGroundLintScripts.log
	CreateGroundScriptsTar.bat GripGroundScripts.tar

GripGroundPictures.tar:  $(LINT) users_ground.dex _check_messages.dex
	copy /Y /V users_ground.dex users.dex
	$(LINT) -noquery -pictures=$(PICTURES) users.dex -pbatch=CreateGroundPicturesTar.bat -log=GripGroundLintPictures.log
	CreateGroundPicturesTar.bat GripGroundPictures.tar

GripGround.md5: GripGroundPictures.tar GripGroundScripts.tar
	$(MD5TREE)  GripGroundPictures.tar GripGroundScripts.tar > GripGround.md5

release_ground: GripGroundScripts.tar GripGroundPictures.tar GripGround.md5 
	$(SOURCE)\DexReleaseScripts.bat GripGround
	echo %date% %time% > $@

######################################################################################################################################

#
# Users
#

users_flight.dex:	$(SOURCE)\DexGenerateSubjects.bat SessionSmallSubjectFlight.dex SessionMediumSubjectFlight.dex SessionLargeSubjectFlight.dex SessionU.dex
	$(SOURCE)\DexGenerateSubjects.bat Flight users_flight.dex

users_ground.dex:	$(SOURCE)\DexGenerateSubjects.bat SessionSmallSubjectGround.dex SessionMediumSubjectGround.dex SessionLargeSubjectGround.dex SessionU.dex
	$(SOURCE)\DexGenerateSubjects.bat Ground users_ground.dex

######################################################################################################################################

#
# Sessions
#

SessionSmallSubjectFlight.dex:	$(STATICSCRIPTS)\SessionSmallSubjectFlight.dex $(PROTOCOLS) DexDynamicsFlightSmall.dex DexSeatedFlightSmall.dex DexSupineFlightSmall.dex DexReducedFlightSmall.dex
	copy /Y $(STATICSCRIPTS)\SessionSmallSubjectFlight.dex .\$@
	$(TOUCH)

SessionMediumSubjectFlight.dex:	$(STATICSCRIPTS)\SessionMediumSubjectFlight.dex $(PROTOCOLS) DexDynamicsFlightMedium.dex DexSeatedFlightMedium.dex DexSupineFlightMedium.dex DexReducedFlightMedium.dex
	copy /Y $(STATICSCRIPTS)\SessionMediumSubjectFlight.dex .\$@
	$(TOUCH)

SessionLargeSubjectFlight.dex:	$(STATICSCRIPTS)\SessionLargeSubjectFlight.dex $(PROTOCOLS) DexDynamicsFlightLarge.dex DexSeatedFlightLarge.dex DexSupineFlightLarge.dex DexReducedFlightLarge.dex
	copy /Y $(STATICSCRIPTS)\SessionLargeSubjectFlight.dex .\$@
	$(TOUCH)

SessionSmallSubjectGround.dex:	$(STATICSCRIPTS)\SessionSmallSubjectBDC.dex $(PROTOCOLS) DexDemoSmall.dex DexPracticeSmall.dex DexDynamicsBDCSmall.dex DexSeatedBDCSmall.dex DexSupineBDCSmall.dex DexReducedBDCSmall.dex
	copy /Y $(STATICSCRIPTS)\SessionSmallSubjectBDC.dex .\$@
	$(TOUCH)

SessionMediumSubjectGround.dex:	$(STATICSCRIPTS)\SessionMediumSubjectBDC.dex $(PROTOCOLS) DexDemoMedium.dex DexPracticeMedium.dex DexDynamicsBDCMedium.dex DexSeatedBDCMedium.dex DexSupineBDCMedium.dex DexReducedBDCMedium.dex
	copy /Y $(STATICSCRIPTS)\SessionMediumSubjectBDC.dex .\$@
	$(TOUCH)

SessionLargeSubjectGround.dex:	$(STATICSCRIPTS)\SessionLargeSubjectBDC.dex $(PROTOCOLS) DexDemoLarge.dex DexPracticeLarge.dex DexDynamicsBDCLarge.dex DexSeatedBDCLarge.dex DexSupineBDCLarge.dex DexReducedBDCLarge.dex
	copy /Y $(STATICSCRIPTS)\SessionLargeSubjectBDC.dex .\$@
	$(TOUCH)

SessionU.dex:	$(STATICSCRIPTS)\SessionUtilitiesOnly.dex $(PROTOCOLS) 
	copy /Y $(STATICSCRIPTS)\SessionUtilitiesOnly.dex .\$@
	$(TOUCH)


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

DexReducedFlightSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReducedFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedFlight.bat Upright Sml > $@
DexReducedFlightMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReducedFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedFlight.bat Upright Med > $@
DexReducedFlightLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReducedFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedFlight.bat Upright Lrg > $@


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

### Reduced BDC Protocol

DexReducedBDCSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReducedBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedBDC.bat Upright Sml > $@
DexReducedBDCMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReducedBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedBDC.bat Upright Med > $@
DexReducedBDCLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateReducedBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedBDC.bat Upright Lrg > $@

### Practice Protocol

DexPracticeSmall.dex: $(SCRIPTS) $(SOURCE)\DexGeneratePractice.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGeneratePractice.bat Upright Sml > $@
DexPracticeMedium.dex: $(SCRIPTS) $(SOURCE)\DexGeneratePractice.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGeneratePractice.bat Upright Med > $@
DexPracticeLarge.dex: $(SCRIPTS) $(SOURCE)\DexGeneratePractice.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGeneratePractice.bat Upright Lrg > $@

### Demo Protocol

DexDemoSmall.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDemo.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDemo.bat Upright Sml > $@
DexDemoMedium.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDemo.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDemo.bat Upright Med > $@
DexDemoLarge.dex: $(SCRIPTS) $(SOURCE)\DexGenerateDemo.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDemo.bat Upright Lrg > $@


# ------------------------------------------------------------------------------------------------
# --            Some generally useful protocols for testing, hardware setup, etc.               --
# ------------------------------------------------------------------------------------------------

### Utilities

ProtocolUtilities.dex: $(STATICSCRIPTS)\ProtocolUtilities.dex TaskCheckLEDs.dex TaskCheckWaitAtTargetSuHo.dex TaskCheckWaitAtTargetSuVe.dex TaskCheckWaitAtTargetUpVe.dex TaskCheckWaitAtTargetUpHo.dex TaskCheckSlip.dex TaskCheckMASS.dex TaskCheckFT.dex TaskAcquire30s.dex TaskAcquire5s.dex calibrate.dex task_align.dex task_nullify.dex task_shutdown.dex 
	copy /Y $(STATICSCRIPTS)\ProtocolUtilities.dex .\$@
	$(TOUCH)
ProtocolInstallUpright.dex: $(STATICSCRIPTS)\ProtocolInstallUpright.dex ForceOffsets.dex TaskInstallUpright.dex 
	copy /Y $(STATICSCRIPTS)\ProtocolInstallUpright.dex .\$@
	$(TOUCH)
ProtocolInstallSupine.dex: $(STATICSCRIPTS)\ProtocolInstallSupine.dex ForceOffsets.dex TaskInstallSupine.dex 
	copy /Y $(STATICSCRIPTS)\ProtocolInstallSupine.dex .\$@
	$(TOUCH)

######################################################################################################################################

#
# Tasks 
#

### Configuration of DEX hardware.

TaskInstallUpright.dex:	$(COMPILER)
	$(COMPILER) -install -upright -compile=$@
TaskInstallSupine.dex:	$(COMPILER)
	$(COMPILER) -install -supine -compile=$@
TaskFinishProtocol.dex: $(COMPILER)
	$(COMPILER) -finish -compile=$@
TaskCheckAudio.dex: $(COMPILER)
	$(COMPILER) -audio -compile=$@
ForceOffsets.dex: $(COMPILER)
	$(COMPILER) -offsets -compile=$@
TaskCheckMASS.dex: $(COMPILER)
	$(COMPILER) -gm -compile=$@

### Utilities

TaskCheckFT.dex: $(COMPILER)
	$(COMPILER) -deploy -ft -compile=$@
TaskCheckSlip.dex: $(COMPILER)
	$(COMPILER) -deploy -slip -compile=$@
TaskCheckLEDs.dex: $(COMPILER)
	$(COMPILER) -LEDs -compile=$@
TaskCheckWaitAtTargetUpVe.dex: $(COMPILER)
	$(COMPILER) -upright -vertical -waits -compile=$@
TaskCheckWaitAtTargetUpHo.dex: $(COMPILER)
	$(COMPILER) -upright -horizontal -waits -compile=$@
TaskCheckWaitAtTargetSuVe.dex: $(COMPILER)
	$(COMPILER) -supine -vertical -waits -compile=$@
TaskCheckWaitAtTargetSuHo.dex: $(COMPILER)
	$(COMPILER) -supine -horizontal -waits -compile=$@
TaskAcquire30s.dex: $(COMPILER)
	$(COMPILER) -rec -duration=30 -compile=$@
TaskAcquire5s.dex: $(COMPILER)
	$(COMPILER) -rec -duration=5 -compile=$@

calibrate.dex: $(STATICSCRIPTS)\calibrate.dex
	copy /Y $(STATICSCRIPTS)\calibrate.dex .
task_align.dex: $(STATICSCRIPTS)\task_align.dex
	copy /Y $(STATICSCRIPTS)\task_align.dex .
task_nullify.dex: $(STATICSCRIPTS)\task_nullify.dex
	copy /Y $(STATICSCRIPTS)\task_nullify.dex .
task_shutdown.dex: $(STATICSCRIPTS)\task_shutdown.dex
	copy /Y $(STATICSCRIPTS)\task_shutdown.dex .
_check_messages.dex: $(STATICSCRIPTS)\_check_messages.dex
	copy /Y $(STATICSCRIPTS)\_check_messages.dex .

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
	echo nothing > release_ground
	del /Q release_ground
	echo nothing > release_flight
	del /Q release_flight



