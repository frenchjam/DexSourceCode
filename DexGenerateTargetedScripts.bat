REM Scripts related to the targeted movements protocols.

REM Select which mass will be used.
set mass=-400

REM Scripts for a small subject.

REM Multiple blocks for vertical movements.

DexSimulatorApp -targeted %mass% -upright -vertical -prep	-targets=TargetedS.1.tgt -compile=TgUpV4S0.dex
DexSimulatorApp -targeted %mass% -upright -vertical			-targets=TargetedS.2.tgt -compile=TgUpV4S1.dex

REM Multiple blocks for horizontal movements.
DexSimulatorApp -targeted %mass% -upright -horizontal	-prep	-targets=TargetedH.1.tgt -compile=TgUpH4S0.dex
DexSimulatorApp -targeted %mass% -upright -horizontal			-targets=TargetedH.2.tgt -compile=TgUpH4S1.dex

REM Repeat for supine position.

REM Multiple blocks for vertical movements.
DexSimulatorApp -targeted %mass% -upright -vertical -prep	-targets=TargetedS.1.tgt -compile=TgSuV4S0.dex
DexSimulatorApp -targeted %mass% -upright -vertical			-targets=TargetedS.2.tgt -compile=TgSuV4S1.dex

REM Multiple blocks for horizontal movements.
DexSimulatorApp -targeted %mass% -upright -horizontal	-prep	-targets=TargetedH.1.tgt -compile=TgUpH4S0.dex
DexSimulatorApp -targeted %mass% -upright -horizontal			-targets=TargetedH.2.tgt -compile=TgUpH4S1.dex


REM Scripts for a large subject.

REM Scripts for a small subject.

REM Multiple blocks for vertical movements.

DexSimulatorApp -targeted %mass% -upright -vertical -prep	-targets=TargetedL.1.tgt -compile=TgUpV4L0.dex
DexSimulatorApp -targeted %mass% -upright -vertical			-targets=TargetedL.2.tgt -compile=TgUpV4L1.dex

REM Multiple blocks for horizontal movements.
DexSimulatorApp -targeted %mass% -upright -horizontal	-prep	-targets=TargetedH.1.tgt -compile=TgUpH4L0.dex
DexSimulatorApp -targeted %mass% -upright -horizontal			-targets=TargetedH.2.tgt -compile=TgUpH4L1.dex

REM Repeat for supine position.

REM Multiple blocks for vertical movements.
DexSimulatorApp -targeted %mass% -upright -vertical -prep	-targets=TargetedL.1.tgt -compile=TgSuV4L0.dex
DexSimulatorApp -targeted %mass% -upright -vertical			-targets=TargetedL.2.tgt -compile=TgSuV4L1.dex

REM Multiple blocks for horizontal movements.
DexSimulatorApp -targeted %mass% -upright -horizontal	-prep	-targets=TargetedH.1.tgt -compile=TgUpH4L0.dex
DexSimulatorApp -targeted %mass% -upright -horizontal			-targets=TargetedH.2.tgt -compile=TgUpH4L1.dex

