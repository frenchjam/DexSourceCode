REM Scripts related to the targeted movements protocols.

REM Select which mass will be used.
set mass=-400

REM Scripts for a small subject.

REM Seated, Vertical.
DexSimulatorApp -targeted %mass% -upright	-vertical		-prep	-targets=TargetedS.0.tgt -compile=TgUpV4S0.dex
DexSimulatorApp -targeted %mass% -upright	-vertical				-targets=TargetedS.1.tgt -compile=TgUpV4S1.dex

REM Seated, Horizontal.
DexSimulatorApp -targeted %mass% -upright	-horizontal		-prep	-targets=TargetedH.0.tgt -compile=TgUpH4S0.dex
DexSimulatorApp -targeted %mass% -upright	-horizontal				-targets=TargetedH.1.tgt -compile=TgUpH4S1.dex

REM Supine, Vertical.
DexSimulatorApp -targeted %mass% -supine	-vertical		-prep	-targets=TargetedS.0.tgt -compile=TgSuV4S0.dex
DexSimulatorApp -targeted %mass% -upright	-vertical				-targets=TargetedS.1.tgt -compile=TgSuV4S1.dex

REM Supine, Horizontal.
DexSimulatorApp -targeted %mass% -supine	-horizontal		-prep	-targets=TargetedH.0.tgt -compile=TgUpH4S0.dex
DexSimulatorApp -targeted %mass% -supine	-horizontal				-targets=TargetedH.1.tgt -compile=TgUpH4S1.dex


REM Scripts for a large subject.

REM Seated, Vertical.
DexSimulatorApp -targeted %mass% -upright	-vertical		-prep	-targets=TargetedL.0.tgt -compile=TgUpV4L0.dex
DexSimulatorApp -targeted %mass% -upright	-vertical				-targets=TargetedL.1.tgt -compile=TgUpV4L1.dex

REM Seated, Horizontal.
DexSimulatorApp -targeted %mass% -upright	-horizontal		-prep	-targets=TargetedH.0.tgt -compile=TgUpH4L0.dex
DexSimulatorApp -targeted %mass% -upright	-horizontal				-targets=TargetedH.1.tgt -compile=TgUpH4L1.dex

REM Supine, Vertical.
DexSimulatorApp -targeted %mass% -supine	-vertical		-prep	-targets=TargetedL.0.tgt -compile=TgSuV4L0.dex
DexSimulatorApp -targeted %mass% -supine	-vertical				-targets=TargetedL.1.tgt -compile=TgSuV4L1.dex

REM Supine, Horizontal.
DexSimulatorApp -targeted %mass% -supine	-horizontal		-prep	-targets=TargetedH.0.tgt -compile=TgUpH4L0.dex
DexSimulatorApp -targeted %mass% -supine	-horizontal				-targets=TargetedH.1.tgt -compile=TgUpH4L1.dex

