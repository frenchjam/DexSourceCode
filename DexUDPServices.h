/* 
 * File:	TrackerUDPServices.h
 * Author:	R. Foster / J. McIntyre
 */

#ifndef _DexUDPServices_

#include <stdio.h>
#include <stdlib.h>

#define DEX_UDP_ERROR -1
#define DEX_UDP_NODATA -2


typedef struct sockaddr_in SockAddrIn;

typedef struct {

  char *device;
  char  *ip_address;
  int   port;

  struct sockaddr_in  sockaddr;
  SOCKET  socket;

  
} DexUDP; 

#define DexUDPPort( t ) ( ntohs( (t)->sockaddr.sin_port ) )
#define DexUDPAddress( t ) ( inet_ntoa( (t)->sockaddr.sin_addr ) )


#ifdef __cplusplus
extern "C" {
#endif 


/* Initialize a link to the tracker UDP server. */
#define DEX_DEFAULT_UDP_PORT    4010
#define DEX_SAMPLES_PER_PACKET	128
#define DEX_FLOATS_PER_SAMPLE		4
#define DEX_UDP_HEADER_SIZE			256
// Each packet can contain a 256 byte header plus a limited number of data points.
#define DEX_UDP_PACKET_SIZE			(DEX_UDP_HEADER_SIZE + DEX_FLOATS_PER_SAMPLE * sizeof( float ) * DEX_SAMPLES_PER_PACKET)

// If I send out UDP packets to soon, one after the other, they get missed.
// I don't know if one gets overwritten by the next on the send side (i.e. the send doesn't block)
//  of if it is that on the receive side it can't keep up.
// After each UDP packet sent I sleep this many milliseconds to work around this problem.
#define DEX_UDP_WAIT						100

int DexUDPInitClient( DexUDP *dex_udp_parameters, char *broadcast_address );

/* Read data server and device. */
unsigned int DexUDPGetPacket( DexUDP *dex_udp_parameters, char *packet );

/*
 * Server analog of client services above. However, the 
 * broadcast address and the port must be specified. In the
 * absence of a valid broadcast address, the server will send
 * packets only to the local host (127.0.0.1).
 */

int DexUDPInitServer( DexUDP *dex_udp_parameters, char *broadcast_address );
unsigned int DexUDPSendPacket( DexUDP *dex_udp_parameters, char *packet );

#ifdef __cplusplus
}
#endif

#define _DexUDPServices_
#endif
