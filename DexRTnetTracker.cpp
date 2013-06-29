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

// RTNet C++ includes
#define NO64BIT
#include "codaRTNetProtocolCPP/RTNetClient.h"
#include "codaRTNetProtocolCPP/DeviceOptionsAlignment.h"
#include "codaRTNetProtocolCPP/DeviceOptionsCodaMode.h"
#include "codaRTNetProtocolCPP/DeviceInfoAlignment.h"
#include "codaRTNetProtocolCPP/PacketDecode3DResult.h"
#include "codaRTNetProtocolCPP/PacketDecode3DResultExt.h"
#include "codaRTNetProtocolCPP/PacketDecodeADC16.h"

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
		for(DWORD iconfig = 0; iconfig < configs.dwNumConfig; iconfig++) {
			fOutputDebugString( "  [%u] address: %08X  name: %s\n", 
				iconfig, configs.config[iconfig].dwAddressHandle, configs.config[iconfig].strName);
		}
		if (configs.dwNumConfig < codaConfig)
		{
			fOutputDebugString("ERROR: specified hardware configuration %d not available.\n", codaConfig);
			exit( -1 );
		}
		// Show which configuration we are trying to use.
		fOutputDebugString( "RTnet Configuration: %s\n", configs.config[codaConfig].strName );
		

		// It used to be that we could just run the startup commands even
		// if the system is already running. Now it seems that calling 
		// PrepareForAcquisition() if it has already been called causes problems.
		// So if we are running already and it is not the same configuration
		// then shut down and start again.
		DWORD	running_config = cl.getRunningHWConfig();
		if ( running_config != configs.config[codaConfig].dwAddressHandle ) {
			OutputDebugString( "*** Not running the right configuration! ***" );
			cl.stopSystem();
			running_config = NULL;
		}


		if ( !running_config ) {

			OutputDebugString( "*** Starting up Coda system! ***" );

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
			OutputDebugString( "cl.prepareForAcq()\n" );
			cl.prepareForAcq();

		}

		// Create a data stream.
		// I know from experience that the port number (here 7000) has to be different
		// for each client that wants to access the RTnet server at the same time.
		// To run different applications simultaneously, I just define a different port
		// number for each one. But what if I want to run the same application on different
		// machines?
		// TO DO: Figure out how the port number is supposed to be used.
		cl.createDataStream(stream, 7001);
		// ensure socket is up before starting acquisition
		codanet_sleep(50);

		// Find out how many Coda units are actually in use.
		// I don't really need the alignment information, but that structure
		// includes the number of Codas specified in the configuration that is in use.
		DeviceInfoAlignment align_info;
		cl.getDeviceInfo( align_info );
		nCodas = align_info.dev.dwNumUnits;
		
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
}

//*
//* Retrieve the stored CX1 data.
//*

int DexRTnetTracker::RetrieveMarkerFrames( CodaFrame frames[], int max_frames ) {
	
	OutputDebugString( "Start Retrieval\n" );

	int nFrames; 
	int frm;
	int unit;
	
	int nChecksumErrors = 0;
	int nTimeouts = 0;
	int nUnexpectedPackets = 0;
	int nFailedFrames = 0;
	int nSuccessfullPackets = 0;
	
	//* Find out how many frames worth of data are stored for this device.
	//* Note that StopAcqusition() should have done this already, but lets be sure.
	//* Note also that the routine says it gets the number of packets, but I believe
	//* that this is a misnomer, since it sends 3 packets per slice of data
	//* (one for each coda unit and one for the combined data).
	nFrames = cl.getAcqBufferNumPackets( cx1Device );
	fOutputDebugString( "\nStart retrieval of Marker data (%d frames) .", nFrames );

	//* Loop through and get them all.
	for ( frm = 0; frm < nFrames && frm < max_frames; frm++ ) {

		fOutputDebugString( "%08u\tTrying.\n", frm );
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
			/*... ignore status information - errors will just cause codanet_packet_receive to time out... */
				OutputDebugString( "Caught error from cl.requestAcqBufferPacket()\n" );
			}
			//* We are supposed to get nCoda + 1 per time slice.
			unit = 0;
			while ( unit <= nCodas ) {
				// Time out means we did not get as many packets as expected.
				// So request them again for this time slice.
				if ( stream.receivePacket(packet, 50000) == CODANET_STREAMTIMEOUT ) {
					nTimeouts++;
					break;
				}
				if ( !packet.verifyCheckSum() ) {
					nChecksumErrors++;
				}
				else if ( decode3D.decode( packet ) ) {
					nUnexpectedPackets++;
				}
				else {
					// Process packet.
					nSuccessfullPackets++;
					// find number of marker positions available
					DWORD nMarkers = decode3D.getNumMarkers();

					fOutputDebugString( "%08u\tOK.", frm );

					// TO DO: Fill the frames[] array.

					// Count the number of packets received for this frame.
					unit++;
				}
			}
		}
		if ( retry >= maxRetries ) {
			nFailedFrames++;
//			fOutputDebugString( "%08u\tfailed.\n", frm );
		}
	}
			
	if ( nFailedFrames || nTimeouts || nChecksumErrors || nUnexpectedPackets ) {
		char message[2048];
		sprintf( message, "Packet Errors:\n\nFrames: %d\nFailed Frames: %d\nTimeouts: %d\nChecksum Errors: %d\nUnexpected Packets: %d",
			nFrames, nFailedFrames, nTimeouts, nChecksumErrors, nUnexpectedPackets );
		MessageBox( NULL, message, "RTnetAcquisition", MB_OK | MB_ICONEXCLAMATION );
	}
	else OutputDebugString( "Stop Retrieval\n" );
	return( nSuccessfullPackets );
	
}


bool DexRTnetTracker::GetCurrentMarkerFrame( CodaFrame &frame ) {
	
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
	
	//* Process packets.
	//* Charnwood version makes no assumptions about how many devices, and therefore how many packets
	//*  will arrive and need to be processed. So they try for packets until they get a timeout.
	//* In my version we assume that there will be 1 packets from each CX1, and one combined.
	//* When all have been received, we can move on without waiting any longer.
	//* This should allow rapid responses to movements of the markers.
	//* Be careful, though, because if the loop runs fast enough you will probably get multiple 
	//*  packets from the same clock tick.
	
	if ( 1 ) {
		
		int received_cx1 = 0;
		do {
			// receive data - wait for max 50ms
			if ( stream.receivePacket(packet, 50000) == CODANET_STREAMTIMEOUT )
			{
				//* A timeout means that we didn't get one of the packets that we expected.
				return( false );
				break;
			}
			
			// check result
			if ( packet.verifyCheckSum() )
			{
				
				// decode & print results
				if ( decode3D.decode(packet) )
				{
					// find number of marker positions available
					DWORD nMarkers = decode3D.getNumMarkers();
					
					// loop through one or more markers (set maxDisplayMarkers to do more than one)
					for (DWORD imarker = 0; imarker < nMarkers; imarker++)
					{
						float   *pos = decode3D.getPosition(imarker);
						BYTE    valid = decode3D.getValid(imarker);
						BYTE    *intensity = decode3D.getIntensity(imarker);
						DWORD   tick = decode3D.getTick();
						
						// TO DO: Do something with the data.
						
					}
					//* Signal that we got the CX1 packet.
					received_cx1++;
				}
			}
			else
			{
				// Signal a checksum  or packet type error.
				// TO DO: Decide what to do if it happens.
				// Here we just ignore and continue.
				fOutputDebugString( "Checksum failed!!!" );
			}
			//* If expected packets received, move on.
		} while ( received_cx1 < 1); 
		// Flush any other packets that might come in.
		while ( stream.receivePacket(packet, 50000) != CODANET_STREAMTIMEOUT ); 
	}
	
	return( true );
	
}

/****************************************************************************************/

bool DexRTnetTracker::GetAcquisitionState( void ) {
	// I believe that this should work, but I did not test it with a CODA.
	return( cl.isAcqInProgress() );
}

int DexRTnetTracker::GetNumberOfCodas( void ) {
	return( nCodas );
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
void DexRTnetTracker::print_alignment_status(const DWORD* marker_id_array,  const DeviceInfoAlignment& info)
{
	// print alignment status value
	fprintf(stderr, "Alignment result: ");
	switch (info.dev.dwStatus)
	{
	case 0:
		fOutputDebugString( "success");
		break;
	case CODANET_ALIGNMENTERROR_SYSTEM:
		fOutputDebugString( "system error");
		break;
	case CODANET_ALIGNMENTERROR_ALREADY_ACQUIRING:
		fOutputDebugString( "already acquiring (is another program running?");
		break;
	case CODANET_ALIGNMENTERROR_OCCLUSIONS:
		fOutputDebugString( "occlusions");
		break;
	case CODANET_ALIGNMENTERROR_XTOOCLOSE:
		fOutputDebugString( "x-axis markers too close");
		break;
	case CODANET_ALIGNMENTERROR_XYTOOCLOSE:
		fOutputDebugString( "xy-plane markers too close");
		break;
	case CODANET_ALIGNMENTERROR_NOTPERP:
		fOutputDebugString( "marked axes not sufficiently perpendicular");
		break;
	default:
		fOutputDebugString( "unknown alignment status error code %d", info.dev.dwStatus);
		break;
	}
	fOutputDebugString( "\n");
	
	// number of CX1 units
	DWORD nunits = info.dev.dwNumUnits;
	
	// frame count
	DWORD nframes = info.dev.dwNumFrames;
	
	// print visibility information
	for (DWORD icoda = 0; icoda < nunits; icoda++)
	{
		// index of Codamotion CX1 unit
		fOutputDebugString( "Coda %d\n", icoda+1);
		
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
					fOutputDebugString( "Org");
					break;
				case 1:
					fOutputDebugString(  "X0 ");
					break;
				case 2:
					fOutputDebugString( "X1 ");
					break;
				case 3:
					fOutputDebugString( "XY0");
					break;
				case 4:
					fOutputDebugString( "XY1");
					break;
				}
				
				// print marker identity
				fOutputDebugString( " (Marker %02d) ", marker_identity);
				
				// camera letter for each coda camera
				switch (icam)
				{
				case 0:
					fOutputDebugString( "A");
					break;
				case 1:
					fOutputDebugString( "B");
					break;
				case 2:
					fOutputDebugString( "C");
					break;
				}
				
				// space
				fOutputDebugString( ": ");
				
				// print visibility graph for frames of data
				// show a 1 for visible and _ for occluded
				// (show alternate frames only to save screen space)
				for (DWORD iframe = 0; iframe < nframes; iframe+=2)
				{
					BYTE flag = info.dev.camera_flag[3*nframes*5*icoda + 3*nframes*imarker + 3*iframe + icam];
					if (flag <= 10)
						fOutputDebugString( "_");
					else
						fOutputDebugString( "1");
				}
				
				// new line
				fOutputDebugString( "\n");
			}
		}
	}
}



