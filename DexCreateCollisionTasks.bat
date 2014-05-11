@echo OFF

REM
REM Create a set of Collision tasks for DEX.
REM

REM Assumes that task, mass, posture, direction,etc. are set already.
set dir=%direction:~0,4%

REM Provide instructions for first block.
REM Subroutine will disable it for subsequent blocks.
set prep=-prep

REM Initialize the block counter. 
REM It gets incremented by the subroutine.
set seq=0

REM Procuce the required number of blocks.
FOR /L %%s IN ( 1,1,%nblocks% ) DO CALL :DO_ONE_COLLISION

REM Return to caller.
goto :EOF

:DO_ONE_COLLISION

	REM Each repetition is a separate task.
	set /A "task=task+1"

	REM Each repetition uses a different target sequence.
	set /A "seq=seq+1"
	set /A "sseq=seq+100"
	set sq=%sseq:~1,2%

	REM Construct a file tag.
	set dir=%direction:~0,1%
	set post=%posture:~0,1%
	set ms=%mass:~0,1%
	set tag=%sz%%task%C%post%%dir%%ms%

	REM Put all the paramters together for the compiler.
	set params=-collisions -%mass% -%posture% -tag=%tag% -delays=CollisionsSequences30.txt:%seq%

	REM Generate a script filename based on the parameters.
	REM set filename=%tag%Col%mass%%sq%.dex
	set filename=%tag%%sq%.dex

	REM Generate the script to do the task.
	%COMPILER% %params%  -compile=%filename% %prep% %*

	REM Record the task in the protocol definition, labelled appropriately.
	echo CMD_TASK,%task%,%filename%,%task% Collisions %sq%

	REM On subsequent calls within the same block, don't redo the prep.
	set prep=

	REM Return to caller.
	goto :EOF
