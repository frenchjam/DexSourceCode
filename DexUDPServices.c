/* 
* File:	DexUDPServices.cpp
* Author:	R. Foster / J. McIntyre
* Rev:		-
* Desc:	
*/
#include <Winsock2.h>

#include "DexUDPServices.h"

static unsigned int defaultPort = DEX_DEFAULT_UDP_PORT;
//static char *defaultServerAddress = "127.0.0.1";

 
// Don't get stuck if you are running without an actual connection.
// We send UDP packets to the local host as well.

static struct sockaddr_in		localXmitAddr;
static int	  LocalAddrSize = sizeof( localXmitAddr );

/************************************************************************/

int DexUDPInitServer( DexUDP *dex_udp_parameters, char *broadcast_address ) {
	
	WORD wVersionRequired = MAKEWORD (2, 2);
	WSADATA wd;
	int iReturn;
	
	// Initialize Winsock
	iReturn = WSAStartup ( wVersionRequired, &wd );
	if (iReturn != 0){
		MessageBox( NULL,"WSAStartup() failed", "TrackerUDPServices", MB_OK ); 
		WSACleanup ();
		return( FALSE );
	};
	
	// Create a socket to send UDP Datagrams
	dex_udp_parameters->socket = WSASocket( AF_INET, SOCK_DGRAM, IPPROTO_UDP, 0, 0, 0 );
	if ( dex_udp_parameters->socket == INVALID_SOCKET){
		MessageBox( NULL,  "WSASocket() failed.", "DexUDPServices", MB_OK );
		WSACleanup ();
		return(FALSE);
	};
	
	// Set up to broadcast to the ethernet network, if a broadcast address was specified.
	if ( broadcast_address ) {
		dex_udp_parameters->sockaddr.sin_addr.s_addr = inet_addr( broadcast_address );	// Broadcast to my laptop JAM
		dex_udp_parameters->sockaddr.sin_family = AF_INET;
		dex_udp_parameters->sockaddr.sin_port = htons( (u_short) DEX_DEFAULT_UDP_PORT );
		
		dex_udp_parameters->ip_address = broadcast_address;
		dex_udp_parameters->port = DEX_DEFAULT_UDP_PORT;
	}
	else {
		dex_udp_parameters->sockaddr.sin_addr.s_addr = NULL;
	}
	
	// Set to send locally in any case.
	localXmitAddr.sin_family = AF_INET;
	localXmitAddr.sin_port = htons( (u_short) DEX_DEFAULT_UDP_PORT );
	localXmitAddr.sin_addr.s_addr = inet_addr( "127.0.0.1" );	
	
	return(TRUE);
	
}



/************************************************************************/

unsigned int DexUDPSendPacket( DexUDP *dex_udp_parameters, char data[DEX_UDP_PACKET_SIZE] )
{
	
	DWORD			dwBytesSent = 0;
	DWORD     dwFlags = 0;
	WSABUF    wbSend;
	
	int       status;
	
	static    count = 0;
	
	wbSend.buf = (char *) data;	
	wbSend.len = DEX_UDP_PACKET_SIZE;
	
	// Send packet to the local host no matter what.
	status = WSASendTo( dex_udp_parameters->socket, 
		&wbSend, 1, 
		&dwBytesSent, 
		dwFlags, 
		(SOCKADDR*) &localXmitAddr, 
		LocalAddrSize,
		NULL, NULL);
	
	// Broadcast to the net only if a valid broadcast address was already set.
	if ( dex_udp_parameters->sockaddr.sin_addr.s_addr ) {
		status = WSASendTo( dex_udp_parameters->socket, 
			&wbSend, 1, 
			&dwBytesSent, 
			dwFlags, 
			(SOCKADDR*) &dex_udp_parameters->sockaddr, 
			sizeof( dex_udp_parameters->sockaddr ),
			NULL, NULL);
	}
	
	if ( status ) {
		status = WSAGetLastError();
		dwBytesSent = 0;
	}
	
	return( dwBytesSent );
	
}


/************************************************************************/

// Listen for data packets from the DEX UDP Server.
// If the calling program specifies a server address, listen to it only.
// Otherwise, listen to packets on this port from any host (see INADDR_ANY below).

int DexUDPInitClient( DexUDP *dex_udp_parameters, char *server_address ){
	
	/* Initialize Winsock */
	WORD wVersionRequired = MAKEWORD (2, 2);
	WSADATA wd;
	int iReturn;
	
	
	iReturn = WSAStartup (wVersionRequired, &wd);
	if (iReturn != 0){
		MessageBox( NULL,  "WSAStartup() failed.", "DexUDPServices", MB_OK );
		WSACleanup (); 
		return( -1 );
	};
	
	/*Create the trackerRecvSocketet */
	dex_udp_parameters->socket = WSASocket( AF_INET, SOCK_DGRAM, IPPROTO_UDP, 0, 0, 0);
	if ( dex_udp_parameters->socket == INVALID_SOCKET ){
		MessageBox( NULL,  "WSASocket() failed.", "DexUDPServices", MB_OK );
		WSACleanup ();
		return( -2 );
	};
	
	/* Set up UDP Socket */
	dex_udp_parameters->sockaddr.sin_family = AF_INET;
	dex_udp_parameters->sockaddr.sin_port = htons( (u_short) DEX_DEFAULT_UDP_PORT );
	if ( server_address ) dex_udp_parameters->sockaddr.sin_addr.s_addr = inet_addr( server_address );
	else dex_udp_parameters->sockaddr.sin_addr.s_addr = htonl( INADDR_ANY );
	
	
	/* Bind Socket to any address on the specified port*/
	if (bind( dex_udp_parameters->socket, (SOCKADDR*) &dex_udp_parameters->sockaddr, sizeof(dex_udp_parameters->sockaddr)) == SOCKET_ERROR)
	{
		MessageBox( NULL,  "bind() failed.", "DexUDPServices", MB_OK );
		closesocket( dex_udp_parameters->socket );
		return( -3 );
	}
	
	return( FALSE );
	
}



/************************************************************************/

unsigned int DexUDPGetPacket( DexUDP *dex_udp_parameters, char data[DEX_UDP_PACKET_SIZE] )
{
	
	DWORD			    dwBytesRecv, available;
	
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
	struct timeval timeout = {0,0};
	
	// Make sure that we read all records in the queue.
	int got_data = FALSE;
	do  {
		
		FD_ZERO( &readfds );
		FD_ZERO( &writefds );
		FD_ZERO( &exceptfds );
		FD_SET( dex_udp_parameters->socket, &readfds );
		
		available = select( 0, &readfds, &writefds, &exceptfds, &timeout );
		
		if ( available == SOCKET_ERROR ) {
			int code = WSAGetLastError();
			char msg[1024];
			sprintf( msg, "Error querying tracker socket.\nCode %d", code );
			MessageBox( NULL, msg, "Dex UDP Client", MB_OK );
			return( DEX_UDP_ERROR );
		}
		else if ( available ) {
			// Here is where we actually read the data packet.
			dwBytesRecv = recv( dex_udp_parameters->socket, (char *) data, DEX_UDP_PACKET_SIZE, 0 );	
			if ( dwBytesRecv != DEX_UDP_PACKET_SIZE ) { 
				char msg[1024];
				sprintf( msg, "Error reading tracker socket.\nBytes received: %d", dwBytesRecv );
				MessageBox( NULL, msg, "Dex UDP Client", MB_OK );
				return( DEX_UDP_ERROR );
			}
			else got_data = TRUE;
		}
		
	} while( available );
	
	if ( got_data ) return( FALSE );
	else return( DEX_UDP_NODATA );
	
}
