#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#include <string.h>
#include <errno.h>
#include <netdb.h>

#include "CClientSocket.hpp"
#include "CTrace.hpp"

using std::cout;
using std::endl;

// Constructor: begin listening on the port:
CClientSocket::CClientSocket( const std::string& serverName, int portNo, \
      CCallbackInterface* cbDoor ) : cbClass( cbDoor ), clientSocket(0)
{
   struct sockaddr_in serverAddress;
   struct hostent *server;
   std::stringstream ss;

   // Open a client socket
   clientSocket = socket( AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0 );
   if( clientSocket < 0 ) 
   {
       ss << "ERROR opening client socket! errno=" << errno;
       throw std::runtime_error( ss.str() );
   }

   // Get server structure
   server = gethostbyname( serverName.c_str() ); 
   if( server == NULL )
   {
       ss << "ERROR getting server host data! errno=" << errno;
       throw std::runtime_error( ss.str() );
   }

   memset( &serverAddress, 0, sizeof( serverAddress ) );
   serverAddress.sin_family = AF_INET;
   memcpy( &serverAddress.sin_addr.s_addr, (void*)server->h_addr, server->h_length );
   serverAddress.sin_port = htons( portNo );

   // Try to connect to server
   if( connect( clientSocket, \
       (struct sockaddr*)&serverAddress, sizeof( struct sockaddr_in ) ) < 0 && \
       errno != EINPROGRESS )
   {
       ss << "ERROR connecting to server! " << strerror( errno );
       throw std::runtime_error( ss.str() );
   }
}

// Destructor: disconnect all clients and shut down sockets:
CClientSocket::~CClientSocket()
{
   close( clientSocket );
}

// Obtain the socket of the connection
int CClientSocket::getSocket( )
{
   return clientSocket;
}

// Accept pending connection requests from clients and store them in the queue
void CClientSocket::manage()
{
   std::stringstream ssSelectTrace;

   int maxsock = clientSocket;
   fd_set  set;
   struct timeval timeout = { 0,0 };

   FD_ZERO( &set );
   FD_SET( clientSocket, &set );

   ssSelectTrace << " - SELECT:FDS:(" << clientSocket << ")" << std::endl;

   int rc = select( maxsock + 1, &set, NULL, NULL, &timeout);

   // If failure, throw an exception:
   if( rc < 0 )
   {
       std::stringstream ss( ssSelectTrace.str() );
       ss << "ERROR waiting for incoming messages! - errno=" << errno << \
              "(" << strerror( errno ) << ")";

       throw std::runtime_error( ss.str() );
   }

   // Check if we have read messages:
#ifdef SOCKDBG
   if( rc > 0 )
   {
      CTrace( cout, TRACE_VERBOSE ) << ssSelectTrace.str() << endl;
   }
#endif

   while( rc > 0 )
   {
#ifdef SOCKDBG
      CTrace( cout, TRACE_VERBOSE ) << " - SELECT:REMAINING " << rc << " EVENTS" << endl;
#endif
      rc--;

      if( FD_ISSET( clientSocket, &set ) )
      {
#ifdef SOCKDBG
          CTrace( cout, TRACE_PROCESS ) << \
              " - - SELECT_RES:incoming data(" << clientSocket << ")" << endl;
#endif

          FD_CLR( clientSocket, &set );

          if( readData( ) == 0 )
          {
              CTrace( cout, TRACE_PROCESS ) << \
                  "-- CLOSED CONNECTION [conn " << clientSocket << "]" << endl;

              close( clientSocket );

              // Notify the application
              cbClass->onConnectionClose( clientSocket );
              clientSocket = 0;
          }
      }
   }
}
   
// Communication management
int CClientSocket::readData( )
{
   // Header of any msg:
   u_int8_t tmp_buffer[ 256 ];

   int rc;

   // Read up to 256 bytes
   rc = recv( clientSocket, tmp_buffer, 256, MSG_NOSIGNAL );

   if( rc > 0 )
   {
      // Send to client
      // Notify the reception of the message
      cbClass->onMessageReceived( clientSocket, tmp_buffer, rc );
   }

   return rc;
}

// Send a block of bytes to server
int CClientSocket::sendBytes( const void* data, size_t len, int flags )
{
   int rc = 0;

   if( len == 0 || clientSocket == 0 ) return 0;

   rc = send( clientSocket, data, len, flags );

   // If error, break connection:
   if( rc <= 0 )
   {
       std::stringstream ss;
       ss << "ERROR writing message! - errno=" << errno <<  " (" << \
             strerror( errno ) << ")" << endl;

       throw std::runtime_error( ss.str() );

       close( clientSocket );
       clientSocket = 0;
   }

   return rc;
}


// Close the current connection to the server
void CClientSocket::closeConnection( )
{
   close( clientSocket );
   clientSocket = 0;
}

