REM Scripts related to the targeted movements protocols.

REM Scripts for a small subject.

REM Select which mass will be used.
set mass=-400

REM Multiple blocks for vertical movements.
DexSimulatorApp -targeted %mass% -upright -vertical -prep	-targets=TargetedUprightS.1.tgt -compile=TgUpV4S0.dex
DexSimulatorApp -targeted %mass% -upright -vertical			-targets=TargetedUprightS.2.tgt -compile=TgUpV4S1.dex

REM Multiple blocks for horizontal movements.
DexSimulatorApp -targeted %mass% -upright -horizontal	-prep	-targets=TargetedUprightS.1.tgt -compile=TgUpH4S0.dex
DexSimulatorApp -targeted %mass% -upright -horizontal			-targets=TargetedUprightS.2.tgt -compile=TgUpH4S1.dex

REM Repeat for supine position.

REM Scripts for a medium subject.

REM Select which mass will be used. Here I am using the same mass for small and medium subjects.
set mass=-400

