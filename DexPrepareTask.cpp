	// Clearly demark this operation in the script file. 
	apparatus->Comment( "################################################################################" );

	apparatus->ShowStatus( "Initiating hardware configuration phase ..." );


	// Tell the subject which configuration should be used.
	// TODO: Move this into a protocol, rather than here in a task, because the subject is likely to do multiple
	//   blocks of trials in the same configuration.
	status = apparatus->fWaitSubjectReady( 
		"Install the Workspace Tablet in the %s Position.\nPlace the Target Mast in the %s position.\nPlace the tapping surfaces in the %s position.",
		PostureString[PostureSeated], TargetBarString[bar_position], TappingSurfaceString[TappingFolded] );
	if ( status == ABORT_EXIT ) exit( status );

	// Check that the tracker is still aligned.
	apparatus->ShowStatus( "Tracker alignment check ..." );
	status = apparatus->CheckTrackerAlignment( alignmentMarkerMask, 5.0, 2, "Coda misaligned!" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	apparatus->ShowStatus( "Hardware configuration check ..." );
	status = apparatus->SelectAndCheckConfiguration( posture, bar_position, DONT_CARE );
	if ( status == ABORT_EXIT ) exit( status );
