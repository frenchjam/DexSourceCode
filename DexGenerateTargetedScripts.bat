REM Scripts related to the targeted movements protocols.

REM Scripts for a small subject.

REM Select which mass will be used.
set mass=-400

REM Multiple blocks for vertical movements.
DexSimulatorApp -targeted %mass% -upright -vertical -targets=TargetedUprightS.1.tgt -compile=TargetedVerticalUprightSmall.1.dex
DexSimulatorApp -targeted %mass% -upright -vertical -targets=TargetedUprightS.2.tgt -compile=TargetedVerticalUprightSmall.2.dex

REM Multiple blocks for horizontal movements.
DexSimulatorApp -targeted %mass% -upright -horizontal -targets=TargetedUprightS.1.tgt -compile=TargetedHorizontalUprightSmall.1.dex
DexSimulatorApp -targeted %mass% -upright -horizontal -targets=TargetedUprightS.2.tgt -compile=TargetedHorisontalUprightSmall.2.dex

REM Repeat for supine position.
DexSimulatorApp -targeted %mass% -supine -vertical -targets=TargetedSupineS.1.tgt -compile=TargetedVerticalSupineSmall.1.dex
DexSimulatorApp -targeted %mass% -supine -vertical -targets=TargetedSupineS.2.tgt -compile=TargetedVerticalSupineSmall.2.dex
DexSimulatorApp -targeted %mass% -supine -horizontal -targets=TargetedSupineS.1.tgt -compile=TargetedHorizontalSupineSmall.1.dex
DexSimulatorApp -targeted %mass% -supine -horizontal -targets=TargetedSupineS.2.tgt -compile=TargetedHorisontalSupineSmall.2.dex

REM Scripts for a medium subject.

REM Select which mass will be used. Here I am using the same mass for small and medium subjects.
set mass=-400

REM Multiple blocks for vertical movements.
DexSimulatorApp -targeted %mass% -upright -vertical -targets=TargetedUprightM.1.tgt -compile=TargetedVerticalUprightMedium.1.dex
DexSimulatorApp -targeted %mass% -upright -vertical -targets=TargetedUprightM.2.tgt -compile=TargetedVerticalUprightMedium.2.dex

REM Multiple blocks for horizontal movements.
DexSimulatorApp -targeted %mass% -upright -horizontal -targets=TargetedUprightM.1.tgt -compile=TargetedHorizontalUprightMedium.1.dex
DexSimulatorApp -targeted %mass% -upright -horizontal -targets=TargetedUprightM.2.tgt -compile=TargetedHorisontalUprightMedium.2.dex

REM Repeat for supine position.
DexSimulatorApp -targeted %mass% -supine -vertical -targets=TargetedSupineM.1.tgt -compile=TargetedVerticalSupineMedium.1.dex
DexSimulatorApp -targeted %mass% -supine -vertical -targets=TargetedSupineM.2.tgt -compile=TargetedVerticalSupineMedium.2.dex
DexSimulatorApp -targeted %mass% -supine -horizontal -targets=TargetedSupineM.1.tgt -compile=TargetedHorizontalSupineMedium.1.dex
DexSimulatorApp -targeted %mass% -supine -horizontal -targets=TargetedSupineM.2.tgt -compile=TargetedHorisontalSupineMedium.2.dex
