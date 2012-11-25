/* 
 * File:	TrackerUDPServices.h
 * Author:	R. Foster / J. McIntyre
 */

#ifndef _TrackerUDPServices_

#include <stdio.h>
#include <stdlib.h>
#include <3dMatrix.h>

/* 
 * Structure for a 6dof data record, compatible with InterSense.
 */

#define TRACKER_STATIONS 3

typedef struct
{
  char			RecordType;
  char			StationNumber;
  char			status[2];			// Extra space to align the structure to an even bounday
  float     position[3];
  float			orientation[4];
  float			timestamp;
} IsenseDataRecord;

typedef struct
{
  IsenseDataRecord			Station[TRACKER_STATIONS];
} IsenseDataPacket;


typedef struct
{
  Pose                  Station[TRACKER_STATIONS];
} UDPDataPacket;


#define TRACKER_UDP_ERROR -1
#define TRACKER_UDP_NODATA -2


typedef struct sockaddr_in SockAddrIn;

typedef struct {

  char *device;

  char  *ip_address;
  int   port;

  struct sockaddr_in  sockaddr;
  SOCKET  socket;

  
} UDPTracker; 

#define UDPTrackerPort( t ) ( ntohs( (t)->sockaddr.sin_port ) )
#define UDPTrackerIPAddress( t ) inet_ntoa( (t)->sockaddr.sin_addr )
#define UDPTrackerDevice( t ) ((t)->device )


#ifdef __cplusplus
extern "C" {
#endif 


/*
 * Initialize a link to the tracker UDP server. 
 * One can pass an IP address (dot notation) or a port number..
 * The address determines the host server while the port determines which 
 * device will actually be used. A single server might provide data from more than
 * one device(see defs above). For instance, the VOILA server will provide both 
 * ISENSE data or 6dof data derived from the CODA marker data directly.
 * Normally these parameters are set to NULL. In this case the 
 * server and device actually used will be determined by the configuration
 * stored in environment variables. In this way one can reconfigure all applications 
 * to switch from one tracker source to another.
 */

#define DEFAULT_PORT    0
#define ISENSE_PORT     4001
#define OPTICAL_PORT    4002
#define SIMULATOR_PORT  4003

int UDPTrackerInitClient( UDPTracker *tracker, char *server_address, unsigned int port );

/* Read 6dof data from said tracker server and device. */
unsigned int UDPTrackerGetIsenseData( UDPTracker *tracker, IsenseDataPacket *data );

/*
 * Server analog of client services above. However, the 
 * broadcast address and the port must be specified. In the
 * absence of a valid broadcast address, the server will send
 * packets only to the local host (127.0.0.1).
 */

unsigned int UDPTrackerSendData( UDPTracker *tracker, UDPDataPacket *data );
unsigned int UDPTrackerGetData( UDPTracker *tracker, UDPDataPacket *data );

int UDPTrackerInitServer( UDPTracker *tracker, char *broadcast_address, unsigned int port );
unsigned int UDPTrackerSendIsenseData( UDPTracker *tracker, IsenseDataPacket *data );

void PoseToIsense( IsenseDataPacket *isense, Pose pose[3] );
void PosesToUDPPacket( UDPDataPacket *data, Pose pose[3] );

#ifdef __cplusplus
}
#endif

#define _TrackerUDPServices_
#endif
