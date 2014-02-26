REM Scripts related to the targeted movements protocols.

REM This script takes an input parameter, which can be sml, med, lrg. It adds this as the extension to the target file list
REM so that the targets can be tuned to different size subject. It also inserts this string into the script file name

IF "%~1"="" let sz = trg
ELSE let sz = %1
let pos = %2

echo sz  %sz
echo pos %pos



REM Select which mass will be used.
set mass=-400

REM Multiple blocks for vertical movements.
DexSimulatorApp -targeted %mass% -upright -vertical -prep	-targets=Targeted%pos.1.%sz -compile=Tg%posV4%sz0.dex
DexSimulatorApp -targeted %mass% -upright -vertical			-targets=Targeted%pos.2.%sz -compile=Tg%pos4V%sz1.dex

REM Multiple blocks for horizontal movements.
DexSimulatorApp -targeted %mass% -upright -horizontal	-prep	-targets=Targeted%pos.1.%sz -compile=Tg%posH4%sz0.dex
DexSimulatorApp -targeted %mass% -upright -horizontal			-targets=Targeted%pos.2.%sz -compile=Tg%posH4%sz1.dex

REM Repeat for supine position.

REM Scripts for a medium subject.

REM Select which mass will be used. Here I am using the same mass for small and medium subjects.
set mass=-400

