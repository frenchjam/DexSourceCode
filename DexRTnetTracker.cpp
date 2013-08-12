/***************************************************************************/
/*                                                                         */
/*                               DexRTnetTracker                           */
/*                                                                         */
/***************************************************************************/

// A tracker that uses the CODA RTnet interface.

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <VectorsMixin.h>

#include <fOutputDebugString.h>
#include "DexTracker.h"

// Starting up the CODA takes time. It would be nice if we could leave it
// in a running state after the first startup, to go faster on subsequent trials.
// Set this flag to force a shutdown before each start up for testing purposes.
#define ALWAYS_SHUTDOWN

// RTNet C++ includes
#define NO64BIT
#include "codaRTNetProtocolCPP/RTNetClient.h"
#include "codaRTNetProtocolCPP/DeviceOptionsAlignment.h"
#include "codaRTNetProtocolCPP/DeviceOptionsCodaMode.h"
#include "codaRTNetProtocolCPP/DeviceInfoAlignment.h"
#include "codaRTNetProtocolCPP/PacketDecode3DResult.h"
#include "codaRTNetProtocolCPP/PacketDecode3DResultExt.h"
#include "codaRTNetProtocolCPP/PacketDecodeADC16.h"
#include "codaRTNetProtocolCPP/DeviceInfoUnitCoordSystem.h"

#define DEVICEID_ADC DEVICEID_GS16AIO

/***************************************************************************/

void DexRTnetTracker::Initialize( void ) {
	
	try {
		
		// Decode the IP address string that has already been initialized as part of the class.
		unsigned int p, q, r, s;
		fOutputDebugString( "RTnet Server: %s   Port: %d  Configuration: %d\n", serverAddress, serverPort, codaConfig );
		sscanf( serverAddress, "%d.%d.%d.%d", &p, &q, &r, &s );
		
		// Connect to the server.
		cl.connect( (p << 24) + (q << 16) + (r << 8) + s, serverPort );
		
		// Check that the server has at least the expected number of configurations.
		cl.enumerateHWConfig(configs);
		
		// Print config names.
		// This won't really work because the names are wide strings while
		//  fOutputDebugString() uses old-fashioned strings.
		// But this stuff should never happen anyway.
		fOutputDebugString( "Found %u hardware configurations:\n", configs.dwNumConfig);
		char dest[1024];
		for(DWORD iconfig = 0; iconfig < configs.dwNumConfig; iconfig++) {
			wcstombs( dest, configs.config[iconfig].strName, sizeof( dest ) );
			fOutputDebugString( "  [%u] address: %08X  name: %s\n", 
				iconfig, configs.config[iconfig].dwAddressHandle, dest );
		}
		if (configs.dwNumConfig < codaConfig)
		{
			fOutputDebugString("ERROR: specified hardware configuration %d not available.\n", codaConfig);
			exit( -1 );
		}
		// Show which configuration we are trying to use.
		wcstombs( dest, configs.config[codaConfig].strName, sizeof( dest ) );
		fOutputDebugString( "RTnet Configuration: %s\n", dest );
		

		// It used to be that we could just run the startup commands even
		// if the system is already running. Now it seems that calling 
		// PrepareForAcquisition() if it has already been called causes problems.
		// So if we are running already and it is not the same configuration
		// then shut down and start again.
		DWORD	running_config = cl.getRunningHWConfig();
#ifdef ALWAYS_SHUTDOWN
		OutputDebugString( "Shutting down ... " );
		cl.stopSystem();
		running_config = NULL;
		OutputDebugString( "OK.\n" );
#else
		if ( running_config != configs.config[codaConfig].dwAddressHandle ) {
			OutputDebugString( "*** Not running the right configuration! ***" );
			cl.stopSystem();
			running_config = NULL;
		}
#endif


		if ( !running_config ) {

			OutputDebugString( "Starting up Coda system ... " );

			// If system is already started, this does nothing.
			// Otherwise it will load the specified configuration.
			// We assume that the configuration constants defined here correspond
			//  to the actual configurations defined on the server..
			cl.startSystem( configs.config[codaConfig].dwAddressHandle );

			// Here we set the marker acquisition frequency, decimation and whether or
			//  not external sync is enabled. The values are set during class initialization.
			cl.setDeviceOptions( mode );
			// This says that we want both combined and individual data from each coda.
			cl.setDeviceOptions( packet_mode );

			// prepare for acquisition
			OutputDebugString( "OK.\ncl.prepareForAcq() ... " );
			cl.prepareForAcq();
			OutputDebugString( "OK.\n" );

		}

		// Find out how many Coda units are actually in use.
		// I don't really need the alignment information, but that structure
		// includes the number of Codas specified in the configuration that is in use.
		// So we do a bogus alignment.
		DeviceOptionsAlignment align(1, 1, 1, 1, 1);
		cl.setDeviceOptions( align );
		// Then this is what tells us how many units are there.
		DeviceInfoAlignment align_info;
		cl.getDeviceInfo( align_info );

		fOutputDebugString( "Number of connected CODA units: %d\n", nCodas = align_info.dev.dwNumUnits );

		// Create a data stream.
		// I know from experience that the port number (here 7000) has to be different
		// for each client that wants to access the RTnet server at the same time.
		// To run different applications simultaneously, I just define a different port
		// number for each one. But what if I want to run the same application on different
		// machines?
		// TODO: Figure out how the port number is supposed to be used.
		cl.createDataStream(stream, 7001);
		// ensure socket is up before starting acquisition
		codanet_sleep(50);
		
	}
	catch(NetworkException& exNet)
	{
		print_network_error(exNet);
		fOutputDebugString( "Caught (NetworkException& exNet)\n" );
		MessageBox( NULL, "DexRTnet() failed.\n(NetworkException& exNet)", "DexMessage", MB_OK );
		exit( -1 );
	}
	catch(DeviceStatusArray& array)
	{
		print_devicestatusarray_errors(array);
		fOutputDebugString( "Caught (DeviceStatusArray& array)\n" );
		MessageBox( NULL, "DexRTnet() failed.\n(DeviceStatusArray& array)", "DexMessage", MB_OK );
		exit( -1 );
	}
		
}

/***************************************************************************/

int DexRTnetTracker::Update( void ) {
	// The Coda tracker does not need any periodic updating.
	return( 0 );
}

/*********************************************************************************/

void DexRTnetTracker::StartAcquisition( float max_duration ) {
	
	overrun = false;
	cl.setAcqMaxTicks(DEVICEID_CX1, floor( max_duration * 200.0 ));
	OutputDebugString( "cl.startAcqContinuousBuffered()\n" );
	cl.startAcqContinuousBuffered();
	
}

bool DexRTnetTracker::CheckAcquisitionOverrun( void ) { return overrun; }

void DexRTnetTracker::StopAcquisition( void ) {
	// stop acquisition if still in progress
	OutputDebugString( "Stop acquisition\n" );
	if ( cl.isAcqInProgress() ) cl.stopAcq();
	//* If the acquisition was already stopped, it means that the Coda
	//* stopped after reaching the defined number of cycles.
	//* Of course, it could also mean that an acquisition was not even started.
	else overrun = true;
	//* Find out how many ticks worth of data are stored for this device.
	//* Note that the routine says it gets the number of packets, but I believe
	//* that this is a misnomer, since it sends 3 packets per slice of data
	//* (one for each coda unit and one for the combined data).
	nAcqFrames = cl.getAcqBufferNumPackets( cx1Device );
	if ( nAcqFrames > DEX_MAX_MARKER_FRAMES ) nAcqFrames = DEX_MAX_MARKER_FRAMES;

	int frm;
	int unit_count;
	
	int nChecksumErrors = 0;
	int nTimeouts = 0;
	int nUnexpectedPackets = 0;
	int nFailedFrames = 0;
	int nSuccessfullPackets = 0;
	
	fOutputDebugString( "\nStart retrieval of Marker data (%d frames).\n", nAcqFrames );

	//* Loop through and get them all.
	for ( frm = 0; frm < nAcqFrames; frm++ ) {

		//* Try to read the packets. Normally they should get here the first try.
		//* But theoretically, they could get lost or the could get corrupted. 
		//* So if we get a time out or checksum errors, we should try again.
		for ( int retry = 0; retry < maxRetries; retry++ ) {
			try
			{
				//* Tell the server to send the specified packet.
				cl.requestAcqBufferPacket( cx1Device, frm);
			}
			catch(DeviceStatusArray&)
			{
				OutputDebugString( "Caught error from cl.requestAcqBufferPacket()\n" );
			}
			//* We are supposed to get nCoda + 1 packets per request.
			unit_count = 0;
			while ( unit_count <= nCodas ) {
				// Time out means we did not get as many packets as expected.
				// So request them again for this time slice.
				if ( stream.receivePacket(packet, 50000) == CODANET_STREAMTIMEOUT ) {
					nTimeouts++;
					break;
				}
				if ( !packet.verifyCheckSum() ) {
					nChecksumErrors++;
					break;
				}
				else if ( !decode3D.decode( packet ) ) {
					nUnexpectedPackets++;
					break;
				}
				else {
					// Process packet.
					nSuccessfullPackets++;
					// find number of marker positions available
					DWORD nMarkers = decode3D.getNumMarkers();
					if ( nMarkers > DEX_MAX_MARKERS ) {
						MessageBox( NULL, "How many markers?!?!", "Dexterous", MB_OK );
						exit( -1 );
					}

					int   unit = decode3D.getPage();
					if ( unit > nCodas ) {
						MessageBox( NULL, "Which unit?!?!", "Dexterous", MB_OK );
						exit( -1 );
					}

					recordedMarkerFrames[unit][frm].time = decode3D.getTick() * cl.getDeviceTickSeconds( DEVICEID_CX1 );
					for ( int mrk = 0; mrk < nMarkers; mrk++ ) {
						float *pos = decode3D.getPosition( mrk );
						for ( int i = 0; i < 3; i++ ) recordedMarkerFrames[unit][frm].marker[mrk].position[i] = pos[i];
						recordedMarkerFrames[unit][frm].marker[mrk].visibility = ( decode3D.getValid( mrk ) != 0 );
					}
					// Count the number of packets received for this frame.
					// There should be one per unit, plus one for the combined.
					unit_count++;
				}
			}
			// If we finished the loop normally that means we got all the packets for this frame.
			// If we got here via one of the break statements, then we should try again.
			if ( unit_count > nCodas ) break;
		}
		if ( retry >= maxRetries ) {
			nFailedFrames++;
//			fOutputDebugString( "%08u\tfailed.\n", frm );
		}
	}
			
	if ( nFailedFrames || nTimeouts || nChecksumErrors || nUnexpectedPackets ) {
		char message[2048];
		sprintf( message, "Packet Errors:\n\nFrames: %d\nFailed Frames: %d\nTimeouts: %d\nChecksum Errors: %d\nUnexpected Packets: %d",
			nAcqFrames, nFailedFrames, nTimeouts, nChecksumErrors, nUnexpectedPackets );
		MessageBox( NULL, message, "RTnetAcquisition", MB_OK | MB_ICONEXCLAMATION );
	}
	else OutputDebugString( "Stop Retrieval\n" );

	// It appears that the system has to be prepared again for new acquisitions.
	// That's not documented anywhere, but it seems to be true.
	// TODO: Verify with Charnwood.
	OutputDebugString( "OK.\ncl.prepareForAcq() ... " );
	cl.prepareForAcq();
	OutputDebugString( "OK.\n" );

}

//*
//* Retrieve the stored CX1 data.
//*

int DexRTnetTracker::RetrieveMarkerFrames( CodaFrame frames[], int max_frames, int unit ) {
	//* Loop through and get them all.
	for ( int frm = 0; frm < nAcqFrames; frm++ ) {
		CopyMarkerFrame( frames[frm], recordedMarkerFrames[unit][frm] );
	}
	return( nAcqFrames );
}


bool DexRTnetTracker::GetCurrentMarkerFrame( CodaFrame &frame ) {
	
	int unit_count;
	
	int nChecksumErrors = 0;
	int nTimeouts = 0;
	int nUnexpectedPackets = 0;
	int nFailedFrames = 0;
	int nSuccessfullPackets = 0;

	bool status = false;

	// If the CODA is already acquiring, just read the latest data.
	// We assume that it is in buffered mode, so we have to ask for the latest frame.
	if ( cl.isAcqInProgress() ) {
		try
		{
			cl.monitorAcqBuffer();
		}
		catch(DeviceStatusArray& array)
		{
			// If an error occured, output some debug info the debug screen.
			// Other than that, this is a fatal error so we exit.
			print_devicestatusarray_errors(array);
			OutputDebugString( "Caught (DeviceStatusArray& array)\n" );
			exit( -1 );
		}
	}
	// If not already acquiring, then request acquisition of a single frame.
	else {
		// request a frame from all devices
		try
		{
			cl.doSingleShotAcq();
		}
		catch(DeviceStatusArray& array)
		{
			print_devicestatusarray_errors(array);
			fOutputDebugString( "Caught (DeviceStatusArray& array)\n" );
			exit( -1 );
		}
	}
	
	// Set a counter to count the number of packets that we get from the request.
	// We are supposed to get nCoda + 1 packets per request.
	unit_count = 0;
	while ( unit_count <= nCodas ) {

		// Time out means we did not get as many packets as expected.
		// So request them again for this time slice.
		if ( stream.receivePacket(packet, 50000) == CODANET_STREAMTIMEOUT ) {
			nTimeouts++;
			break;
		}

		// Check if the packet is corrupted.
		if ( !packet.verifyCheckSum() ) {
			nChecksumErrors++;
			break;
		}

		// Check if it is a 3D marker packet. It could conceivably  be a packet
		// from the ADC device, although we don't plan to use the CODA ADC at the moment.
		else if ( !decode3D.decode( packet ) ) {
			nUnexpectedPackets++;
			break;
		}

		// If we get this far, it is a valid marker packet.
		else {
			// Count the total number of valid packets..
			nSuccessfullPackets++;
			// find number of markers included in the packet.
			DWORD n_markers = decode3D.getNumMarkers();

			// Single shots can return 56 marker positions, even if we are using
			// 200 Hz / 28 markers for continuous acquisition. Stay within bounds.
			if ( n_markers > DEX_MAX_MARKERS ) {
				n_markers = DEX_MAX_MARKERS;
			}
			
			// The 'page' number is used to say which CODA unit the packet belongs to.
			// TODO: Double check that page 0 is the combined data.
			int   unit = decode3D.getPage();
			if ( unit > nCodas ) {
				// I don't believe that we should ever get here, but who knows?
				MessageBox( NULL, "Which unit?!?!", "Dexterous", MB_OK );
				exit( -1 );
			}
			
			// For realtime monitoring we take only the combined data.
			if ( unit == 0 ) {

				// Compute the time from the tick counter in the packet and the tick duration.
				// Actually, I am not sure if the tick is defined on a single shot acquistion.
				frame.time = decode3D.getTick() * cl.getDeviceTickSeconds( DEVICEID_CX1 );

				// Get the marker data from the CODA packet.
				for ( int mrk = 0; mrk < n_markers; mrk++ ) {
					float *pos = decode3D.getPosition( mrk );
					for ( int i = 0; i < 3; i++ ) frame.marker[mrk].position[i] = pos[i];
					frame.marker[mrk].visibility = ( decode3D.getValid( mrk ) != 0 );
				}

				// If the packet contains fewer markers than the nominal number for
				//  the apparatus, set the other markers to be out of sight..
				for ( mrk = mrk; mrk < nMarkers; mrk++ ) {
					float *pos = decode3D.getPosition( mrk );
					for ( int i = 0; i < 3; i++ ) frame.marker[mrk].position[i] =INVISIBLE;
					frame.marker[mrk].visibility = false;
				}

				// Signal that we got the data that we were seeking.
				status = true;
			}
			// Count the number of packets received for this frame.
			// There should be one per unit, plus one for the combined.
			unit_count++;
		}
	}

	return( status );
	
}

/****************************************************************************************/

bool DexRTnetTracker::GetAcquisitionState( void ) {
	// I believe that this should work, but I did not test it with a CODA.
	return( cl.isAcqInProgress() );
}

int DexRTnetTracker::GetNumberOfCodas( void ) {
	return( nCodas );
}

int  DexRTnetTracker::PerformAlignment( int origin, int x_negative, int x_positive, int xy_negative, int xy_positive ) {

	// Get what are the alignment transformations before doing the alignment.
	// This is just for debugging. Set a breakpoint to see the results.
	DeviceInfoUnitCoordSystem pre_xforms;
	cl.getDeviceInfo( pre_xforms );

	DeviceOptionsAlignment align( origin + 1, x_negative + 1, x_positive + 1, xy_negative + 1, xy_positive + 1);
	cl.setDeviceOptions( align );

	// Show what are the alignment transformations after doing the alignment.
	// This is just for debugging. Set a breakpoint to see the results.
	DeviceInfoUnitCoordSystem post_xforms;
	cl.getDeviceInfo( post_xforms );

	// retrieve information
	DeviceInfoAlignment info;
	cl.getDeviceInfo(info);

	// print alignment diagnostics
	DWORD marker_id_array[5] = { origin + 1, x_negative + 1, x_positive + 1, xy_negative + 1, xy_positive + 1 };
	int response = print_alignment_status(marker_id_array, info);

	if ( info.dev.dwStatus == 0 ) return( NORMAL_EXIT );
	else return( ERROR_EXIT );
}

/*********************************************************************************/

void DexRTnetTracker::GetUnitTransform( int unit, Vector3 &offset, Matrix3x3 &rotation ) {

	DeviceInfoUnitCoordSystem coord;
	cl.getDeviceInfo( coord );

	CopyVector( offset, coord.dev.Rt[unit].t );
	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			// If this seems backwards to you, it's because RTnet uses column vectors
			// and I use row vectors. So when RTnet does M * v, I need to do v * M'.
			rotation[i][j] = coord.dev.Rt[unit].R[j*3+i];
		}
	}

}



/****************************************************************************************/

// Helper Functions

// print_devicestatusarray_errors
// Helper function to print device errors (server-side errors)
// @param array Reference to DeviceStatusArray object thrown as an exception by RTNetClient
void DexRTnetTracker::print_devicestatusarray_errors(const DeviceStatusArray& array)
{
	for (DWORD idev = 0; idev < array.numstatus; idev++)
	{
		if (array.status[idev].error)
		{
			fOutputDebugString( "DEVICE %u SUBSYSTEM %u ERROR: %u\n", 
				(DWORD)array.status[idev].deviceID, 
				(DWORD)array.status[idev].subsystemID,
				(DWORD)array.status[idev].error);
		}
	}
}

// print_network_error
// Helper function to network errors (client-side errors)
// @param exNet Network exception object thrown from RTNetClient or DataStream
void DexRTnetTracker::print_network_error(const NetworkException& exNet)
{
	fOutputDebugString( "Network error: ");
	switch (exNet.errorcode)
	{
	case CODANET_OK:
		fOutputDebugString( "CODANET_OK: NO ERROR CODE WAS PRODUCED");
		break;
		
	case CODANET_SOCKETERROR_BROKEN:
		fOutputDebugString( "CODANET_SOCKETERROR_BROKEN");
		break;
		
	case CODANET_SOCKETERROR_WINDOWSDLL:
		fOutputDebugString( "CODANET_SOCKETERROR_WINDOWSDLL");
		break;
		
	case CODANET_SOCKETERROR_CREATE:
		fOutputDebugString( "CODANET_SOCKETERROR_CREATE");
		break;
		
	case CODANET_SOCKETERROR_HOSTNAME:
		fOutputDebugString( "CODANET_SOCKETERROR_HOSTNAME");
		break;
		
	case CODANET_SOCKETERROR_CONNECT:
		fOutputDebugString( "CODANET_SOCKETERROR_CONNECT");
		break;
		
	case CODANET_SOCKETERROR_TCPOPTIONS:
		fOutputDebugString( "CODANET_SOCKETERROR_TCPOPTIONS");
		break;
		
	case CODANET_CLIENTPROTOCOLERROR_BADNUMSTATUS:
		fOutputDebugString( "CODANET_CLIENTPROTOCOLERROR_BADNUMSTATUS");
		break;
		
	case CODANET_SOCKETERROR_STREAMCREATE:
		fOutputDebugString( "CODANET_SOCKETERROR_STREAMCREATE");
		break;
		
	case CODANET_SOCKETERROR_STREAMPORT:
		fOutputDebugString( "CODANET_SOCKETERROR_STREAMPORT");
		break;
		
	case CODANET_CLIENTPROTOCOLERROR_TOOBIG:
		fOutputDebugString( "CODANET_CLIENTPROTOCOLERROR_TOOBIG");
		break;
		
	default:
		fOutputDebugString( "UNKNOWN ERROR CODE %u", codanet_getlasterror());
		break;
	}
	
}

// print_alignment_status
// Helper function to show alignment status
// @param marker_id_array 5-element array containing one-based marker identities for alignment markers
// @param info Alignment info retrieved from server
int DexRTnetTracker::print_alignment_status(const DWORD* marker_id_array,  const DeviceInfoAlignment& info)
{

	char message[10240] = "";
	char line[1024];
	// print alignment status value
	strcpy( message, "Alignment result: ");
	switch (info.dev.dwStatus)
	{
	case 0:
		strcat( message, "success");
		break;
	case CODANET_ALIGNMENTERROR_SYSTEM:
		strcat( message,  "system error");
		break;
	case CODANET_ALIGNMENTERROR_ALREADY_ACQUIRING:
		strcat( message,  "already acquiring (is another program running?");
		break;
	case CODANET_ALIGNMENTERROR_OCCLUSIONS:
		strcat( message,  "occlusions");
		break;
	case CODANET_ALIGNMENTERROR_XTOOCLOSE:
		strcat( message,  "x-axis markers too close");
		break;
	case CODANET_ALIGNMENTERROR_XYTOOCLOSE:
		strcat( message,  "xy-plane markers too close");
		break;
	case CODANET_ALIGNMENTERROR_NOTPERP:
		strcat( message,  "marked axes not sufficiently perpendicular");
		break;
	default:
		sprintf( line, "unknown alignment status error code %d", info.dev.dwStatus);
		strcat( message, line );
		break;
	}
	strcat( message, "\n");
	
	// number of CX1 units
	DWORD nunits = info.dev.dwNumUnits;
	
	// frame count
	DWORD nframes = info.dev.dwNumFrames;
	
	// print visibility information
	for (DWORD icoda = 0; icoda < nunits; icoda++)
	{
		// index of Codamotion CX1 unit
		sprintf( line, "Coda %d\n", icoda+1);
		strcat( message, line );
		
		// data from each marker
		for (DWORD imarker = 0; imarker < 5; imarker++)
		{
			// actual marker identity
			DWORD marker_identity = marker_id_array[imarker];
			
			for (DWORD icam = 0; icam < 3; icam++)
			{
				// label for this marker
				switch (imarker)
				{
				case 0:
					strcat( message, "Org");
					break;
				case 1:
					strcat( message,  "X0  ");
					break;
				case 2:
					strcat( message, "X1  ");
					break;
				case 3:
					strcat( message, "XY0");
					break;
				case 4:
					strcat( message, "XY1");
					break;
				}
				
				// print marker identity
				sprintf( line, " (Marker %02d) ", marker_identity);
				strcat( message, line );
				
				// camera letter for each coda camera
				switch (icam)
				{
				case 0:
					strcat( message, "A");
					break;
				case 1:
					strcat( message, "B");
					break;
				case 2:
					strcat( message, "C");
					break;
				}
				
				// space
				strcat( message, ": ");
				
				// print visibility graph for frames of data
				// show a 1 for visible and _ for occluded
				// (show alternate frames only to save screen space)
				for (DWORD iframe = 0; iframe < nframes; iframe+=2)
				{
					BYTE flag = info.dev.camera_flag[3*nframes*5*icoda + 3*nframes*imarker + 3*iframe + icam];
					if (flag <= 10)
						strcat( message, "-");
					else
						strcat( message, "1");
				}
				
				// new line
				strcat( message, "\n");
			}
		}
	}
	if ( info.dev.dwStatus ) {
		int response = MessageBox( NULL, message, "Coda RTnet Alignment", MB_ABORTRETRYIGNORE );
		return( response );
	}
	else return( NORMAL_EXIT );

}



