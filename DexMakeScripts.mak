# This makefile is used to make it easy to run the script generating batch file from within Virtual Studio.
# Building (F7) the DexScripts project will cause this makefile to be executed, which in turn calls 
#  the DexGenerate*.bat batch files to be executed.
# This makefile and the batch files that do the work are kept in DexSourceCode so that it gets distributed in Git.

# Uncomment one of these options or make up your own.
# F7 will build the first 'default:' specification that it finds.
# Careful - 'default' there can be no spaces before 'default:'
#
#default:	clean				# Pick this if you want to clear out all the .dex files as rebuild everything.
#default:	flight_debug
#default:	flight_draft
default:	flight_release
#default:	ground_debug
#default:	ground_draft
#default:	ground_release
#default:	parabolic_debug
#default:	parabolic_draft

# This is here to catch an error where you did not pick a default build.
default_default:
	@echo **********************************************************************************************************************************
	@echo You need to pick the build that you want.
	@echo See beginning of DexMakeScripts.mak.
	@echo **********************************************************************************************************************************
COMPILER	= DexSimulatorApp.exe
SOURCE		= ..\DexSourceCode
DESTINATION	= ..\GripReleases

SCRIPTS		= TaskCheckAudio.dex TaskFinishProtocol.dex null_task.dex
PROTOCOLS	= ProtocolInstallUpright.dex ProtocolInstallSupine.dex ProtocolUtilities.dex ProtocolSensorTests.dex

HELPERS		= $(SOURCE)\DexCreateFrictionTask.bat $(SOURCE)\DexCreateInstallTask.bat $(SOURCE)\DexCreateOffsetTask.bat $(SOURCE)\DexCreateOscillationTask.bat $(SOURCE)\DexCreateDiscreteTask.bat $(SOURCE)\DexCreateTargetedTasks.bat $(SOURCE)\DexCreateCollisionTasks.bat

# The following the path to hand-edited scripts. 
STATICSCRIPTS	= ..\DexSourceCode
# The following the path to picture files sources.
PICTURES		= ..\DexPictures
# The place to put the proofs.
PROOFS			= ..\GripScreenShots

LINT	= ..\DexLint\debug\DexLint.exe
TAR		=	"C:\Program Files\GnuWin32\bin\tar.exe"
MD5TREE	=	..\bin\MD5Tree.exe

TOUCH = copy /b $@ +,,

######################################################################################################################################

flight_debug: users_flight.dex _check_messages.dex
	copy /Y /V users_flight.dex users.dex
	$(SOURCE)\DexReleaseScripts.bat GripFlight Debug

flight_draft: $(LINT) flight_debug
	$(SOURCE)\DexReleaseScripts.bat GripFlight Draft

flight_release: $(LINT) clean flight_debug
	$(SOURCE)\DexReleaseScripts.bat GripFlight Release
	 
ground_debug: users_ground.dex _check_messages.dex
	copy /Y /V users_ground.dex users.dex
	$(SOURCE)\DexReleaseScripts.bat GripGround Debug

ground_draft: $(LINT) ground_debug
	$(SOURCE)\DexReleaseScripts.bat GripGround Draft

ground_release: $(LINT) clean ground_debug
	$(SOURCE)\DexReleaseScripts.bat GripGround Release
	 
parabolic_debug: users_parabolic.dex _check_messages.dex
	copy /Y /V users_parabolic.dex users.dex

parabolic_draft: $(LINT) parabolic_debug
	$(SOURCE)\DexReleaseScripts.bat GripParabolic Draft


######################################################################################################################################

#
# Users
#

users_flight.dex:	$(SOURCE)\DexGenerateSubjects.bat SessionSmallSubjectFlight.dex SessionMediumSubjectFlight.dex SessionLargeSubjectFlight.dex SessionU.dex
	$(SOURCE)\DexGenerateSubjects.bat Flight users_flight.dex

users_ground.dex:	$(SOURCE)\DexGenerateSubjects.bat SessionDemo.dex SessionSmallSubjectTraining.dex SessionMediumSubjectTraining.dex SessionLargeSubjectTraining.dex SessionSmallSubjectBDC.dex SessionMediumSubjectBDC.dex SessionLargeSubjectBDC.dex SessionU.dex
	$(SOURCE)\DexGenerateSubjects.bat BDC users_ground.dex

users_parabolic.dex:	$(SOURCE)\users_parabolic.dex SessionU.dex PFSession1.dex PFSession2.dex PFSession3.dex PFSession4.dex PFSession5.dex PFSession6.dex PFSession7.dex PFSession1.dex PFSession8.dex PFSession9.dex
	copy /Y $(STATICSCRIPTS)\users_parabolic.dex .

######################################################################################################################################

#
# Sessions
#

SessionSmallSubjectFlight.dex:	$(STATICSCRIPTS)\SessionSmallSubjectFlight.dex $(PROTOCOLS)  CommSupineSmall.dex CommUprightSmall.dex DexDynamicsFlightSmall.dex DexSeatedFlightSmall.dex DexSupineFlightSmall.dex DexReducedFlightSmall.dex
	copy /Y $(STATICSCRIPTS)\SessionSmallSubjectFlight.dex .\$@
	$(TOUCH)

SessionMediumSubjectFlight.dex:	$(STATICSCRIPTS)\SessionMediumSubjectFlight.dex $(PROTOCOLS) CommSupineMedium.dex CommUprightMedium.dex DexDynamicsFlightMedium.dex DexSeatedFlightMedium.dex DexSupineFlightMedium.dex DexReducedFlightMedium.dex
	copy /Y $(STATICSCRIPTS)\SessionMediumSubjectFlight.dex .\$@
	$(TOUCH)

SessionLargeSubjectFlight.dex:	$(STATICSCRIPTS)\SessionLargeSubjectFlight.dex $(PROTOCOLS) CommSupineLarge.dex CommUprightLarge.dex DexDynamicsFlightLarge.dex DexSeatedFlightLarge.dex DexSupineFlightLarge.dex DexReducedFlightLarge.dex
	copy /Y $(STATICSCRIPTS)\SessionLargeSubjectFlight.dex .\$@
	$(TOUCH)

SessionSmallSubjectBDC.dex:		$(STATICSCRIPTS)\SessionSmallSubjectBDC.dex $(PROTOCOLS) DexDynamicsBDCSmall.dex DexSeatedBDCSmall.dex DexSupineBDCSmall.dex DexReducedBDCSmall.dex
	copy /Y $(STATICSCRIPTS)\SessionSmallSubjectBDC.dex .\$@
	$(TOUCH)

SessionMediumSubjectBDC.dex:	$(STATICSCRIPTS)\SessionMediumSubjectBDC.dex $(PROTOCOLS) DexDynamicsBDCMedium.dex DexSeatedBDCMedium.dex DexSupineBDCMedium.dex DexReducedBDCMedium.dex
	copy /Y $(STATICSCRIPTS)\SessionMediumSubjectBDC.dex .\$@
	$(TOUCH)

SessionLargeSubjectBDC.dex:		$(STATICSCRIPTS)\SessionLargeSubjectBDC.dex $(PROTOCOLS) DexDynamicsBDCLarge.dex DexSeatedBDCLarge.dex DexSupineBDCLarge.dex DexReducedBDCLarge.dex
	copy /Y $(STATICSCRIPTS)\SessionLargeSubjectBDC.dex .\$@
	$(TOUCH)

SessionSmallSubjectTraining.dex:	$(STATICSCRIPTS)\SessionSmallSubjectTraining.dex $(PROTOCOLS) DexDynamicsTrainingSmall.dex DexSeatedTrainingSmall.dex DexSupineTrainingSmall.dex DexReducedTrainingSmall.dex
	copy /Y $(STATICSCRIPTS)\SessionSmallSubjectTraining.dex .\$@
	$(TOUCH)

SessionMediumSubjectTraining.dex:	$(STATICSCRIPTS)\SessionMediumSubjectTraining.dex $(PROTOCOLS) DexDynamicsTrainingMedium.dex DexSeatedTrainingMedium.dex DexSupineTrainingMedium.dex DexReducedTrainingMedium.dex
	copy /Y $(STATICSCRIPTS)\SessionMediumSubjectTraining.dex .\$@
	$(TOUCH)

SessionLargeSubjectTraining.dex:	$(STATICSCRIPTS)\SessionLargeSubjectTraining.dex $(PROTOCOLS) DexDynamicsTrainingLarge.dex DexSeatedTrainingLarge.dex DexSupineTrainingLarge.dex DexReducedTrainingLarge.dex
	copy /Y $(STATICSCRIPTS)\SessionLargeSubjectTraining.dex .\$@
	$(TOUCH)

SessionDemo.dex:	$(STATICSCRIPTS)\SessionDemo.dex $(PROTOCOLS) DexUprightDemo.dex DexSupineDemo.dex 
	copy /Y $(STATICSCRIPTS)\SessionDemo.dex .\$@
	$(TOUCH)

SessionFlightOnGround.dex:	$(STATICSCRIPTS)\SessionFlightOnGround.dex $(PROTOCOLS)  DexSeatedFlightOnGround.dex DexSupineFlightOnGround.dex DexDynamicsFlightOnGround.dex
	copy /Y $(STATICSCRIPTS)\SessionFlightOnGround.dex .\$@
	$(TOUCH)

SessionU.dex:	$(STATICSCRIPTS)\SessionUtilitiesOnly.dex $(PROTOCOLS) 
	copy /Y $(STATICSCRIPTS)\SessionUtilitiesOnly.dex .\$@
	$(TOUCH)

PFSession1.dex:  $(PROTOCOLS) $(SCRIPTS) $(SOURCE)\DexGeneratePFProtocol.bat
	$(SOURCE)\DexGeneratePFProtocol.bat PF1 > $@

PFSession2.dex:  $(PROTOCOLS) $(SCRIPTS) $(SOURCE)\DexGeneratePFProtocol.bat
	$(SOURCE)\DexGeneratePFProtocol.bat PF2 > $@

PFSession3.dex:  $(PROTOCOLS) $(SCRIPTS) $(SOURCE)\DexGeneratePFProtocol.bat
	$(SOURCE)\DexGeneratePFProtocol.bat PF3 > $@

PFSession4.dex:  $(PROTOCOLS) $(SCRIPTS) $(SOURCE)\DexGeneratePFProtocol.bat
	$(SOURCE)\DexGeneratePFProtocol.bat PF4 > $@

PFSession5.dex:  $(PROTOCOLS) $(SCRIPTS) $(SOURCE)\DexGeneratePFProtocol.bat
	$(SOURCE)\DexGeneratePFProtocol.bat PF5 > $@

PFSession6.dex:  $(PROTOCOLS) $(SCRIPTS) $(SOURCE)\DexGeneratePFProtocol.bat
	$(SOURCE)\DexGeneratePFProtocol.bat PF6 > $@

PFSession7.dex:  $(PROTOCOLS) $(SCRIPTS) $(SOURCE)\DexGeneratePFProtocol.bat
	$(SOURCE)\DexGeneratePFProtocol.bat PF7 > $@

PFSession8.dex:  $(PROTOCOLS) $(SCRIPTS) $(SOURCE)\DexGeneratePFProtocol.bat
	$(SOURCE)\DexGeneratePFProtocol.bat PF8 > $@

PFSession9.dex:  $(PROTOCOLS) $(SCRIPTS) $(SOURCE)\DexGeneratePFProtocol.bat
	$(SOURCE)\DexGeneratePFProtocol.bat PF9 > $@


######################################################################################################################################

# -----------------------------------------------------------------------------------------------------------
# -- Protocols Flight (Dynamics Flight; Reference Seated Flight; Reference Supine Flight ; Flight Reduced) --
# -----------------------------------------------------------------------------------------------------------

### Dynamics Flight Protocol

DexDynamicsFlightSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateDynamicsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsFlight.bat Upright Sml > $@
DexDynamicsFlightMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateDynamicsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsFlight.bat Upright Med > $@
DexDynamicsFlightLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateDynamicsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsFlight.bat Upright Lrg > $@

### Reference Seated Flight Protocol

DexSeatedFlightSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsFlight.bat Upright Sml > $@
DexSeatedFlightMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsFlight.bat Upright Med > $@
DexSeatedFlightLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsFlight.bat Upright Lrg > $@

### Reference Supine Flight Protocol

DexSupineFlightSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsFlight.bat Supine Sml > $@
DexSupineFlightMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsFlight.bat Supine Med > $@
DexSupineFlightLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsFlight.bat Supine Lrg > $@

### Reduced Flight

DexReducedFlightSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateReducedFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedFlight.bat Upright Sml > $@
DexReducedFlightMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateReducedFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedFlight.bat Upright Med > $@
DexReducedFlightLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateReducedFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedFlight.bat Upright Lrg > $@

### Commissioning

CommUprightSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateCommissioningProtocol.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateCommissioningProtocol.bat Upright Sml > $@
CommUprightMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateCommissioningProtocol.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateCommissioningProtocol.bat Upright Med > $@
CommUprightLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateCommissioningProtocol.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateCommissioningProtocol.bat Upright Lrg > $@

CommSupineSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateCommissioningProtocol.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateCommissioningProtocol.bat Supine Sml > $@
CommSupineMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateCommissioningProtocol.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateCommissioningProtocol.bat Supine Med > $@
CommSupineLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateCommissioningProtocol.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateCommissioningProtocol.bat Supine Lrg > $@


# ------------------------------------------------------------------------------------------------
# -- Protocols BDC (Dynamics BDC; Reference Seated BDC; Reference Supine BDC ; Return Reduced) --
# ------------------------------------------------------------------------------------------------

### Dynamics BDC Protocol

DexDynamicsBDCSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateDynamicsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsBDC.bat Upright Sml > $@
DexDynamicsBDCMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateDynamicsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsBDC.bat Upright Med > $@
DexDynamicsBDCLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateDynamicsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsBDC.bat Upright Lrg > $@

### Reference Seated BDC Protocol

DexSeatedBDCSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsBDC.bat Upright Sml > $@
DexSeatedBDCMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsBDC.bat Upright Med > $@
DexSeatedBDCLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsBDC.bat Upright Lrg > $@

### Reference Supine BDC Protocol

DexSupineBDCSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsBDC.bat Supine Sml > $@
DexSupineBDCMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsBDC.bat Supine Med > $@
DexSupineBDCLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsBDC.bat Supine Lrg > $@

### Reduced BDC Protocol

DexReducedBDCSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateReducedBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedBDC.bat Upright Sml > $@
DexReducedBDCMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateReducedBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedBDC.bat Upright Med > $@
DexReducedBDCLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateReducedBDC.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedBDC.bat Upright Lrg > $@

### Dynamics Training Protocol

DexDynamicsTrainingSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateDynamicsTraining.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsTraining.bat Upright Sml > $@
DexDynamicsTrainingMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateDynamicsTraining.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsTraining.bat Upright Med > $@
DexDynamicsTrainingLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateDynamicsTraining.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsTraining.bat Upright Lrg > $@

### Reference Seated Training Protocol

DexSeatedTrainingSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsTraining.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsTraining.bat Upright Sml > $@
DexSeatedTrainingMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsTraining.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsTraining.bat Upright Med > $@
DexSeatedTrainingLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsTraining.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsTraining.bat Upright Lrg > $@

### Reference Supine Training Protocol

DexSupineTrainingSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsTraining.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsTraining.bat Supine Sml > $@
DexSupineTrainingMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsTraining.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsTraining.bat Supine Med > $@
DexSupineTrainingLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateReferentialsTraining.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReferentialsTraining.bat Supine Lrg > $@

### Reduced Training Protocol

DexReducedTrainingSmall.dex: $(COMPILER) $(SOURCE)\DexGenerateReducedTraining.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedTraining.bat Upright Sml > $@
DexReducedTrainingMedium.dex: $(COMPILER) $(SOURCE)\DexGenerateReducedTraining.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedTraining.bat Upright Med > $@
DexReducedTrainingLarge.dex: $(COMPILER) $(SOURCE)\DexGenerateReducedTraining.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateReducedTraining.bat Upright Lrg > $@

### Demo Protocol

DexUprightDemo.dex: $(COMPILER) $(SOURCE)\DexGenerateDemo.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDemo.bat Upright Med > $@
DexSupineDemo.dex: $(COMPILER) $(SOURCE)\DexGenerateDemo.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDemo.bat Supine Med > $@

### Flight Evaluation Protocol
DexDynamicsFlightOnGround.dex: $(COMPILER) $(SOURCE)\DexGenerateDynamicsFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateDynamicsFlight.bat Upright Med 400gm > $@
DexSeatedFlightOnGround.dex: $(COMPILER) $(SOURCE)\DexGenerateSeatedlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateSeatedlight.bat Upright Med 400gm > $@
DexSupineFlightOnGround.dex: $(COMPILER) $(SOURCE)\DexGenerateSupineFlight.bat $(HELPERS) $(SCRIPTS)
	$(SOURCE)\DexGenerateSupineFlight.bat Supine  Med 400gm > $@

# ------------------------------------------------------------------------------------------------
# --            Some generally useful protocols for testing, hardware setup, etc.               --
# ------------------------------------------------------------------------------------------------

### Utilities

ProtocolUtilities.dex: $(STATICSCRIPTS)\ProtocolUtilities.dex TaskCheckLEDs.dex TaskCheckWaitAtTargetSuHo.dex TaskCheckWaitAtTargetSuVe.dex TaskCheckWaitAtTargetUpVe.dex TaskCheckWaitAtTargetUpHo.dex TaskCheckSlip.dex TaskCheckMASS.dex TaskSelfTest.dex TaskCheckFT.dex TaskAcquire30s.dex TaskAcquire5s.dex calibrate.dex task_align.dex task_nullify.dex task_shutdown.dex 
	copy /Y $(STATICSCRIPTS)\ProtocolUtilities.dex .\$@
	$(TOUCH)
ProtocolInstallUpright.dex: $(STATICSCRIPTS)\ProtocolInstallUpright.dex ForceOffsets.dex TaskInstallUpright.dex 
	copy /Y $(STATICSCRIPTS)\ProtocolInstallUpright.dex .\$@
	$(TOUCH)
ProtocolInstallSupine.dex: $(STATICSCRIPTS)\ProtocolInstallSupine.dex ForceOffsets.dex TaskInstallSupine.dex 
	copy /Y $(STATICSCRIPTS)\ProtocolInstallSupine.dex .\$@
	$(TOUCH)
ProtocolSensorTests.dex: $(COMPILER) $(SOURCE)\DexGenerateSensorTests.bat
	$(SOURCE)\DexGenerateSensorTests.bat > $@

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
	$(COMPILER) -offsets -deploy -compile=$@

### Utilities

TaskCheckMASS.dex: $(COMPILER)
	$(COMPILER) -gm -compile=$@
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
TaskSelfTest.dex: $(COMPILER)
	$(COMPILER) -selftest -compile=$@

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
null_task.dex: $(STATICSCRIPTS)\null_task.dex
	copy /Y $(STATICSCRIPTS)\null_task.dex .


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
	echo nothing > __dummy.md5
	del /Q *.md5
	echo nothing > __dummy.tar
	del /Q *.tar



