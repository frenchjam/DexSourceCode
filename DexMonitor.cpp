/*********************************************************************************/
/*                                                                               */
/*                                   DexMonitor.cpp                              */
/*                                                                               */
/*********************************************************************************/

// Contains 2 modules that work together.

// One is the DexMonitorServer. It provide a mechanism for the DEX apparatus
// to transmit data to the ground or to a remote computer.
// There are three flavors: UDP based, pipe based and sending to a local GUI.

// The second implements a graphical interface that receives the transmitted
// data and displays it on the screen.

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 
#include <memory.h>
#include <process.h>

#include <fOutputDebugString.h>


#include <useful.h>
#include <screen.h>
#include <3dMatrix.h>
#include <Timers.h>

#include <CodaUtilities.h>

#include <gl/gl.h>
#include <gl/glu.h>


#include "OpenGLUseful.h"
#include "OpenGLColors.h"
#include "OpenGLWindows.h"
#include "OpenGLObjects.h"
#include "OpenGLViewpoints.h"
#include "OpenGLTextures.h"

#include "AfdObjects.h"
#include "CodaObjects.h"
#include "DexGlObjects.h"
#include "Dexterous.h"

#include <OglDisplayInterface.h>
#include <Views.h>
#include <Layouts.h>


#include "DexMonitor.h"
#include "DexMonitorServer.h"

#include "DexUDPServices.h"
#include "DexSimulatorGUI.h"

// #define AUTOSCALE

float plot_min = -200.0;
float plot_max =  800.0;

/*********************************************************************************/

// This implements the graphical monitoring of the data that is sent from DEX.

DexMonitor::DexMonitor( int screen_left, int screen_top, int n_vertical_targets, int n_horizontal_targets, int n_codas ) {
	
	nTargets = n_vertical_targets + n_horizontal_targets;
	nCodas = n_codas;
	nAcqFrames = 0;

	int screen_width = 600;
	int screen_height = 768;
	
	missed_frames = false;
	
	
	// Create a window for an OpenGL application. 
	
	window = new OpenGLWindow();
	window->Border = true; 
	window->FullScreen = false;
	
	if ( window->Create( NULL, "DEX Monitor - Real Time Display", 
		screen_left, screen_top + screen_height / 3 + 50, 
		screen_width, screen_height - (screen_height / 3) - 100  ) ) {
		
		// Create sets the new window to be the active window.
		// Setup the lights and materials for that window.

		glDefaultLighting();
		glDefaultMaterial();
		window->Activate();
	}  
	
	// Shade model
	glDisable(GL_TEXTURE_2D);							// Enable Texture Mapping ( NEW )
	glEnable(GL_LIGHTING);
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearDepth(1.0f);									  // Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
	
	// Create the virtual environment.
	// We use some objects from the Affordances experiment.
	
	RoomTextures tex;
	
	tex.ceiling = new Texture( "bmp\\light.bmp", 1000, 1000 );
	tex.floor = new Texture( "bmp\\prairie.bmp", 1000, 1000 );
	tex.left = new Texture( "bmp\\epm.bmp", 1000, 2000 );
	tex.right = new Texture( "bmp\\epm.bmp", 1000, 1600 );
	tex.front = 0;
	tex.rear = 0;
	room = new ISS( SLIDING, &tex );
	room->SetPosition( 0, -1000, -4000 );
	room->viewpoint->fov = 60.0;
	room->SetCeilingHeight( 2000 );
	room->SetFloorHeight( 0 );
	room->SetViewingHeight( 900 );
	room->SetAperture( 1800 );
	room->SetViewingDistance( 1200 );
	room->viewpoint->fov = 50.0;
	room->viewpoint->SetOrientation( -5.0, 25.0, -15.0 );
	room->viewpoint->SetOffset( 500.0, 0.0, 0.0 );
	
	room->dial->Disable();
	room->meter_stick->Disable();
	room->indicator->Disable();
	
	for ( int i = 0; i < nCodas; i++ ) {
		coda[i] = new CODA();
		coda[i]->SetColor( GRAY );
		coda[i]->frustrum->Disable();
		coda[i]->SetAttitude( -90.0, 0.0, 0.0 );
	}
	coda[0]->SetOrientation( 0.0, 0.0, 75.0 );
	coda[0]->SetPosition( -900, 0.0, -3000.0 );
	
	coda[1]->SetOrientation( 0.0, 0.0, 180 - 75 );
	coda[1]->SetPosition( 900, 0.0, -3000.0 );
	
	targets = new DexVirtualTargetBar( 300, n_vertical_targets, n_horizontal_targets );
	targets->SetPosition( 0, 0.0, 0.0 );

	manipulandum = new Slab( 20.0, 30.0, 30.0 );
	manipulandum->SetColor( BLUE );
	
	StopAcquisition();
	
	display = DefaultDisplay();
	DisplaySetSizePixels( display, screen_width, screen_height / 3 );
	DisplaySetScreenPosition( display, screen_left, screen_top );
	DisplaySetName( display, "DEX Monitor - Recorded Data" );
	DisplayInit( display );
	Erase( display );
	
	xz_view = CreateView( display );
	ViewSetDisplayEdgesRelative( xz_view, 0.01, 0.01, 0.30, 0.99 );
	ViewSetEdges( xz_view, plot_min, plot_min, plot_max, plot_max );
	ViewMakeSquare(xz_view);
	
	layout = CreateLayout( display, 3, 1 );
	LayoutSetDisplayEdgesRelative( layout, 0.31, 0.01, 0.99, 0.99 );
	
	DexUDPInitClient( &udp_parameters, NULL );
	
}

/*********************************************************************************/

// Move the virtual manipulandum to the indicated position.

void DexMonitor::SetManipulandumPosition( float pos[3] ) {
	manipulandum->SetPosition( pos[X], pos[Y], pos[Z] );
}

/*********************************************************************************/

// Show the CODA sensors in idle or acquisition mode.

void DexMonitor::StartAcquisition( void ) {
	for ( int i = 0; i < nCodas; i++ ) coda[i]->Acquire( true );
}

void DexMonitor::StopAcquisition( void ) {
	for ( int i = 0; i < nCodas; i++ ) coda[i]->Acquire( false );
}

/*********************************************************************************/

// Turn targets off and on.

void DexMonitor::TargetsOff( void ) {
	targets->TargetsOff();
}

void DexMonitor::TargetOn( int id ) {
	targets->TargetOn( id );
}

/*********************************************************************************/

// Make sounds.

void DexMonitor::Beep( void ) {
	fprintf( stderr, "\007" );
	fflush( stderr );
}
void DexMonitor::Boop( void ) {
	fprintf( stderr, "\007\007" );
	fflush( stderr );
}

/*********************************************************************************/

// Display traces of manipulandum movement.

void DexMonitor::PlotAcquisition( float *position, float *orientation, 
								 int frames, float availability_code ) {
	
	double p[3] = { 0.2, 0.0, 1.0 };
	
	ActivateOglWindow();
	Erase( display );
	
	
	if ( missed_frames ) ViewColor( xz_view, RED );
	else ViewColor( xz_view, GREY4 );
	
	ViewBox( xz_view );
	
	if ( frames > 0 ) {
		ViewColor( xz_view, RED );
		ViewPenSize( xz_view, 5 );
		ViewXYPlotAvailableFloats( xz_view,
			&position[Z], 
			&position[Y], 
			0, frames - 1, 
			sizeof( *position ) * 3, 
			sizeof( *position ) * 3, availability_code );
	}
	
	for ( int i = 0; i < 3; i++ ) {
		
		view = LayoutViewN( layout, i );
		ViewColor( view, GREY4 );	
		ViewBox( view );
		
		if ( frames > 0 ) {

#ifdef AUTOSCALE
			ViewSetXLimits( view, 0.0, frames );
			ViewAutoScaleInit( view );
			ViewAutoScaleAvailableFloats( view,
				&position[i], 
				0, frames - 1, 
				sizeof( *position ) * 3, 
				availability_code );
			ViewAutoScaleExpand( view, 0.05 );
#else
			ViewSetXLimits( view, 0.0, frames );
			ViewSetYLimits( view, plot_min, plot_max );
#endif
			
			
			ViewColor( view, BLUE );
			ViewPlotAvailableFloats( view,
				&position[i], 
				0, frames - 1, 
				sizeof( *position ) * 3, 
				availability_code );
			
		}
	}
	
	DisplaySwap( display );
	
}

/*********************************************************************************/

// Upate OpenGL 3D graphics view.

void DexMonitor::Draw( void ) {
	
	window->Activate();
	room->viewpoint->Apply( window, CYCLOPS );
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	window->Clear( BLACK );
	
	room->Draw();
	for ( int i = 0; i < nCodas; i++ ) coda[i]->Draw();
	targets->Draw();	
	manipulandum->Draw();
	
	window->Swap();
	
}

// Plot the data.

void DexMonitor::Plot( void ) {
	PlotAcquisition( recordedPosition, recordedOrientation, nAcqFrames, INVISIBLE );
}

/*********************************************************************************/

// Parse packets of data sent by DexMonitorServer.

void DexMonitor::ParseInputPacket( char *packet ) {
	
	int	updateCounter;
	int	n_codas, n_targets;
	int	acquisition_state;
	int	visibility;
	unsigned long	target_state;
	
	DexTargetBarConfiguration		target_bar_config;
	DexSubjectPosture				subject_posture;
	DexTappingSurfaceConfiguration	tapping_surface_config;
	
	float x, y, z;
	float pos[3], ori[4];
	
	int   frames, sample;
	int   i, j;
	char  token[256];
	
	sscanf( packet, "%s", token );
	
	if ( !strcmp( token, "DEX_QUIT" ) ) exit( 0 );
	
	if ( !strcmp( token, "DEX_CONFIGURATION" ) ) {
		int items = sscanf( packet, "%s %8u %1d %2d [%d %d %d]", 
			token, &updateCounter, &n_codas, &n_targets, &subject_posture, &target_bar_config, &tapping_surface_config );
		
		if ( items < 7 ) {
			char msg[1024];
			sprintf( msg, "Items: %d\n%s", items, packet );
			MessageBox( NULL, msg, "DexMonitor", MB_OK );
			exit( 0 );
		}
		targets->SetConfiguration( subject_posture, target_bar_config, tapping_surface_config );
		
	}
	
	if ( !strcmp( token, "DEX_EVENT" ) ) {

		char *event_message;
		char datestr[256], timestr[256];
		int items = sscanf( packet, "%s %8u %s %s", token, &updateCounter, datestr, timestr );

		event_message = strchr( packet, '*' );
		if ( !event_message ) event_message = "???";
		
		// Display the text of the event.
		// Should log them to a file as well.

		// Take only the first line of the error message.
//		for ( char *ptr = event_message; *ptr; ptr++ ) {
//			if ( *ptr == '\n' ) {
//				*ptr = 0;
//				break;
//			}
//		}
		// Add a time stamp and print it.
		fprintf( stderr, "%s %s %s\n", datestr, timestr, event_message );
		fflush( stderr );
		
		// Debugging
		if ( items < 4 ) {
			char msg[1024];
			sprintf( msg, "Items: %d\n%s", items, packet );
			MessageBox( NULL, msg, "DexMonitor", MB_OK );
			exit( 0 );
		}
	}
	
	if ( !strcmp( token, "DEX_STATE" ) ) {

		int items = sscanf( packet, "%s %d (%d) | %lx | %d < %f %f %f > [ %f %f %f %f ]",
			token, &updateCounter, &acquisition_state, &target_state,
			&visibility, &pos[X], &pos[Y], &pos[Z], &ori[X], &ori[Y], &ori[Z], &ori[M] );
		
		if ( acquisition_state ) StartAcquisition();
		else StopAcquisition();
		
		targets->SetTargetState( target_state ) ;

		if ( visibility ) SetManipulandumPosition( pos );
		
		if ( items < 12 ) {
			char msg[1024];
			sprintf( msg, "Items: %d\n%s", items, packet );
			MessageBox( NULL, msg, "DexMonitor Error", MB_OK );
			exit( 0 );
		}
	}
	
	if ( !strcmp( token, "DEX_RECORDING_START" ) ) {
		nAcqFrames = 0;
		sscanf( packet, "%s %d %d", token, &updateCounter, &frames );
		fprintf( stderr, "Start receiving recording. %d %d\n", updateCounter, frames );
	}
	
	if ( !strcmp( token, "DEX_RECORDING_END" ) ) {
		sscanf( packet, "%s %d %d", token, &updateCounter, &frames );
		if ( frames == nAcqFrames ) missed_frames = false;
		else missed_frames = true;
		fprintf( stderr, "End   receiving recording. %d %d %d Missed frames: %s\n", 
			updateCounter, frames, nAcqFrames, (missed_frames ? "YES" : "NO" ) );
		
	}
	
	// Receive one slice of data.
	if ( !strcmp( token, "DEX_RECORDING_SAMPLE" ) ) {
		sscanf( packet, "%s %d %f %f %f", token, &sample, &x, &y, &z );
		recordedPosition[nAcqFrames*3 + X] = x;
		recordedPosition[nAcqFrames*3 + Y] = y;
		recordedPosition[nAcqFrames*3 + Z] = z;
		nAcqFrames++;
	}
	
	if ( !strcmp( token, "DEX_RECORDING_RECORD" ) ) {
		
		int frames_in_packet;
		sscanf( packet, "%s %d", token, &frames_in_packet );
		fprintf( stderr, "%s %d %d\n", token, frames_in_packet, nAcqFrames );
		
		float *ptr = (float *) (packet + DEX_UDP_HEADER_SIZE);
		j = 0;
		for ( i = 0, j = 0; i < frames_in_packet; i++, j += DEX_FLOATS_PER_SAMPLE ) {
			recordedTime[nAcqFrames] = ptr[j];
			recordedPosition[nAcqFrames * 3 + X] = ptr[j+1];
			recordedPosition[nAcqFrames * 3 + Y] = ptr[j+2];
			recordedPosition[nAcqFrames * 3 + Z] = ptr[j+3];
			nAcqFrames ++;
		}	
	}
}

/*********************************************************************************/

void DexMonitor::ParseInputStream( FILE *fp ) {
	
	char  line[1024];
	
	while ( !feof( fp ) ) {
		
		if ( NULL != fgets( line, sizeof( line ), fp ) ) {
			ParseInputPacket( line );
		}
	}
}

/*********************************************************************************/

int DexMonitor::Update( void ) {
	
	int i = 0;
	int exit_status = NORMAL_EXIT;
	
	char packet[DEX_UDP_PACKET_SIZE];
	
	while( DexUDPGetPacket( &udp_parameters, packet ) != DEX_UDP_NODATA ) {
		ParseInputPacket( packet );
	}
	
	if ( display ) {
		if ( RunOglWindowOnce() ) exit_status = ESCAPE_EXIT;
	}
	int input = window->GetInput( 0.001 );
	if ( input == WM_QUIT ) {
		exit_status = ESCAPE_EXIT;
	}
	
	Plot();
	Draw();
	
	return( exit_status );
	
}

/*********************************************************************************/

void DexMonitor::Quit( void ) {
	
	window->Destroy();
	
}

/*********************************************************************************/

void DexMonitor::RunWindow( void ) {
	
	int exit_status = 0;
	while ( 1 ) {
		
		if ( exit_status = Update() ) {
			// Shutdown the 2D graphics window.
			ShutdownOglWindow();
			// Shutdown the OpenGL 3D window.
			window->Destroy();
			// This kills everything - not very nice!
			exit( exit_status );
		}
		Sleep( 100 );
	}
}


/*********************************************************************************/
/*                                                                               */
/*                                  DexMonitorServer                             */               
/*                                                                               */
/*********************************************************************************/

// Send out data during the experiment execution, to be monitored on the ground.

DexMonitorServer::DexMonitorServer( int n_vertical_targets, int n_horizontal_targets, int n_codas ) {
	
	messageCounter = 0;

}

/*********************************************************************************/

// Send out a record with the recorded manipulandum movement.
// The receiver will display it to the experimenters.

void DexMonitorServer::SendRecording( ManipulandumState state[], int samples, float availability_code ) {
	
	char packet[DEX_UDP_PACKET_SIZE];
	float *ptr;

	int skip = samples / DEX_SAMPLES_PER_PACKET + 1;
	int samples_to_send = samples / skip + 1;

	// Send out on a data stream. Could be stdout or a pipe to another process.
	sprintf( packet, "DEX_RECORDING_START %8u %d", messageCounter, samples_to_send );
	SendPacket( packet );
		
	// Send out a subset of samples to the UDP broadcast.
	int sample = 0;
	while ( sample < samples ) {
		ptr = (float *) (packet + DEX_UDP_HEADER_SIZE);
		int samples_in_packet = 0;
		while ( sample < samples && ptr < (float *) ( packet + sizeof( packet ) ) ) {
			*ptr++ = state[sample].time;
			*ptr++ = state[sample].position[X];
			*ptr++ = state[sample].position[Y];
			*ptr++ = state[sample].position[Z];
			sample += skip;
			samples_in_packet++;
		}
		sprintf( packet, "DEX_RECORDING_RECORD %8d", samples_in_packet );
		SendPacket( packet );
	}

	// Signal that we are done on both streams.
	sprintf( packet, "DEX_RECORDING_END %8u %d", messageCounter, samples_to_send );
	SendPacket( packet );
	
	messageCounter++;
	
}

/*********************************************************************************/

int DexMonitorServer::SendConfiguration( int nCodas, int nTargets, 
										DexSubjectPosture subjectPosture,
										DexTargetBarConfiguration targetBarConfig, 
										DexTappingSurfaceConfiguration tappingSurfaceConfig ) {
	
	char packet[DEX_UDP_PACKET_SIZE];
	int i = 0;
	int exit_status = NORMAL_EXIT;
	
	// Send a packet with the current state of the apparatus.
	
	sprintf( packet, "DEX_CONFIGURATION %8u %1d %2d [%d %d %d]", 
		messageCounter, nCodas, nTargets, 
		subjectPosture, targetBarConfig, tappingSurfaceConfig );
	SendPacket( packet );
	
	messageCounter++;
	
	return( exit_status );
	
}

int DexMonitorServer::SendState( bool acquisitionState, unsigned long targetState, 
								 bool manipulandum_visibility, 
								 Vector3 manipulandum_position, 
								 Quaternion manipulandum_orientation ) {
	
	int i = 0;
	int exit_status = NORMAL_EXIT;
	unsigned long bit = 0x01;
	char packet[DEX_UDP_PACKET_SIZE];
	
	// Send a packet with the current state of the apparatus.	
	sprintf( packet, 
		"DEX_STATE %8u (%1d) | 0x%08x | %1d < %.3f %.3f %.3f >  [ %.3f %.3f %.3f %.3f ] ", 
		messageCounter, acquisitionState, targetState,
		manipulandum_visibility, manipulandum_position[X],  manipulandum_position[Y],  manipulandum_position[Z],
		manipulandum_orientation[X],  manipulandum_orientation[Y],  manipulandum_orientation[Z], manipulandum_orientation[M] );
	SendPacket( packet );
	
	messageCounter++;
	
	return( exit_status );
	
}

int DexMonitorServer::SendEvent( const char* format, ... ) {
	
	va_list args;
	char message[1024];
	
	va_start(args, format);
	vsprintf(message, format, args);
	va_end(args);
	
	int i = 0;
	int exit_status = NORMAL_EXIT;
	
	struct tm *newtime;
	long ltime;
	char datestr[32], timestr[32];
	char packet[DEX_UDP_PACKET_SIZE];
	
	time( &ltime );
	newtime = localtime( &ltime );
	sprintf( datestr, "%4d/%02d/%02d", newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday );
	sprintf( timestr, "%02d:%02d:%02d", newtime->tm_hour, newtime->tm_min, newtime->tm_sec );
	
	// Create a timestamped event.
	sprintf( packet, "DEX_EVENT %8u %s %s * %s", messageCounter, datestr, timestr, message );

	// Take out any newlines.
	for ( char *ptr = packet; *ptr && ptr < packet + sizeof( packet ); ptr++ ) if ( *ptr == '\n' ) *ptr = '|';
	
	// Broadcast the event.
	SendPacket( packet );

	// Show it in the Debug window.
	fOutputDebugString( "%s\n", packet );
	
	messageCounter++;
	
	return( exit_status );
	
}


/*********************************************************************************/

// Signal to the receiver that we are done.

void DexMonitorServer::Quit( void ) {
	
	char packet[DEX_UDP_PACKET_SIZE];
	sprintf( packet, "DEX_QUIT\n" );
	SendPacket( packet );
	
}

/*********************************************************************************/

// Send out packets via UDP, to be monitored on the ground.

DexMonitorServerUDP::DexMonitorServerUDP( int n_vertical_targets, int n_horizontal_targets, int n_codas ) {
	
	messageCounter = 0;
	DexUDPInitServer( &udp_parameters, NULL );
	Sleep( 1000 );
	
}

void DexMonitorServerUDP::SendPacket( const char *packet ) {
	// Broadcast the event.
	DexUDPSendPacket( &udp_parameters, packet );
	Sleep( DEX_UDP_WAIT );
}

/*********************************************************************************/

// Send out packets via the DEX GUI.

DexMonitorServerGUI::DexMonitorServerGUI( int n_vertical_targets, int n_horizontal_targets, int n_codas ) {}

void DexMonitorServerGUI::SendPacket( const char *packet ) {
	// Broadcast the event.
	DexAddToLogGUI( packet );
}

/*********************************************************************************/

// Send out packets via the DEX GUI.

DexMonitorServerPIPE::DexMonitorServerPIPE( int n_vertical_targets, int n_horizontal_targets, int n_codas ) {

	// For demonstration purposes, we pipe the data packets to a program that will display
	//  them. In the real thing, the packets should be sent to ground and displayed there.
	fp = stdout;
		
}

void DexMonitorServerPIPE::SendPacket( const char *packet ) {
	// Broadcast the event.
	fprintf( fp, "%s\n", packet );
	fflush( fp );
}

