#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <string.h>
#include <errno.h>

#include "CServerSocket.hpp"
#include "CTrace.hpp"

using std::cout;
using std::endl;

// Constructor: begin listening on the port:
CServerSocket::CServerSocket( int portNo, CCallbackInterface* cbDoor ) : \
       acceptActive( false ), cbClass( cbDoor )
{
   // Opent listening socket:
   listenSocket = socket( AF_INET, SOCK_STREAM, 0 );
   if( listenSocket < 0 ) 
   {
       std::stringstream ss;
       ss << "ERROR opening listening socket! errno=" << errno;
       throw std::runtime_error( ss.str() );
   }

   // Socket options:
   fcntl( listenSocket, F_SETFL, O_NONBLOCK );

   // Bind to the address/port specified:
   struct sockaddr_in serverAddress;

   memset( &serverAddress, 0, sizeof( serverAddress ) );
   serverAddress.sin_family = AF_INET;
   serverAddress.sin_addr.s_addr = htonl( INADDR_ANY );
   serverAddress.sin_port = htons( portNo );
 
   if( bind( listenSocket, ( struct sockaddr *)&serverAddress, sizeof( serverAddress ) ) < 0 )
   {
       std::stringstream ss;
       ss << "ERROR binding to port " << portNo << "! errno=" << errno << \
          " (" << strerror( errno ) << ")" << std::endl;
       throw std::runtime_error( ss.str() );
   }

   // Start listening for connections:
   if( listen( listenSocket, 10 ) < 0 )
   {
       std::stringstream ss;
       ss << "ERROR listening for connections on port " << portNo << "! errno=" << errno;
       throw std::runtime_error( ss.str() );
   }
}

// Destructor: disconnect all clients and shut down sockets:
CServerSocket::~CServerSocket()
{
   close( listenSocket );
   removeAllConnections();
}

// Accept pending connection requests from clients and store them in the queue
void CServerSocket::manage()
{
   std::stringstream ssSelectTrace;

   int maxsock = listenSocket;
   fd_set  set;
   struct timeval timeout = { 0,0 };

   FD_ZERO( &set );
   FD_SET( listenSocket, &set );

   ssSelectTrace << " - SELECT:FDS:(" << listenSocket;

   // Include the client connections to the set:
   std::deque< int >::iterator dequeit = connectedSockets.begin();
   while( dequeit != connectedSockets.end() )
   {
      ssSelectTrace << "," << *dequeit;

      FD_SET( (*dequeit), &set );

      if( *dequeit > listenSocket ) maxsock = *dequeit;

      dequeit++;
   }

   ssSelectTrace << ")" << std::endl;

   int rc = select( maxsock + 1, &set, NULL, NULL, &timeout);

   // If failure, throw an exception:
   if( rc < 0 )
   {
       std::stringstream ss( ssSelectTrace.str() );
       ss << "ERROR checking for incoming messages or new connections! - errno=" << errno << \
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

      // Check if we have a new connection request:
      if( FD_ISSET( listenSocket, &set ) )
      {
         try {
            __accept_internal();
         } catch( std::runtime_error& e )
         {
             CTrace( cout, TRACE_ERROR ) << (char*)e.what() << endl;
             break;
         };

         FD_CLR( listenSocket, &set );
      }

      // Check if we have activity on a connected socket:
      dequeit = connectedSockets.begin();
      while( dequeit != connectedSockets.end() )
      {
#ifdef SOCKDBG
          CTrace( cout, TRACE_PROCESS ) << " - - SELECT_RES:test(" << *dequeit << ")" << endl;
#endif

          if( FD_ISSET( (*dequeit), &set ) )
          {
#ifdef SOCKDBG
              CTrace( cout, TRACE_PROCESS ) << \
                  " - - SELECT_RES:incoming data(" << *dequeit << ")" << endl;
#endif

              FD_CLR( *dequeit, &set );

              if( readData( (*dequeit) ) == 0 )
              {
                 CTrace( cout, TRACE_PROCESS ) << \
                    "-- CLIENT CLOSED CONNECTION [conn " << *dequeit << "]" << endl;

                 close( *dequeit );

                 // Notify the application
                 cbClass->onConnectionClose( *dequeit );

                 // Erase this connection
                 dequeit = connectedSockets.erase( dequeit );
              }
          }
          else
          {
              dequeit++;
          }
      }
   }
}
   
// Accept a connection:
void CServerSocket::__accept_internal()
{
   struct sockaddr_in cliAddress;
   socklen_t cliLen;

   cliLen = sizeof( struct sockaddr_in );
   memset( &cliAddress, 0, cliLen );

   int newClientSocket = accept( listenSocket, (struct sockaddr*)&cliAddress, &cliLen );

   if( newClientSocket < 0 )
   {
       std::stringstream ss;
       ss << "*ERROR* accepting a connection! errno=" << errno << " (" << strerror(errno) << ")" << endl;
       throw std::runtime_error( ss.str() );
   }

   // Once accepted, we close it if we have the connection acception disabled
   if( acceptActive == false )
   {
      close( newClientSocket );

      std::stringstream ss;
      ss << "-- CLIENT CONNECTION ATTEMPT FROM " << inet_ntoa( cliAddress.sin_addr) << \
            " REJECTED - accept inactive - [have " << connectedSockets.size() << " connections]" << endl;

      CTrace( cout, TRACE_ERROR ) << ss.str() << endl;
   }
   else
   {
      // Add new connection to the list:
      openedSocket( newClientSocket );

      std::stringstream ss;
      ss << "-- NEW CLIENT CONNECTED FROM " << inet_ntoa( cliAddress.sin_addr) << " (fd:" << newClientSocket << \
            ")- [have " << connectedSockets.size() << " connections]" << endl;

      CTrace( cout, TRACE_PROCESS ) << ss.str() << endl;

      // Notify the application
      cbClass->onNewConnection( newClientSocket );
   }
}

// Communication management
int CServerSocket::readData( int sockNumber )
{
   // Header of any msg:
   u_int8_t tmp_buffer[ 256 ];

   int rc;

   rc = recv( sockNumber, tmp_buffer, sizeof(tmp_buffer), MSG_NOSIGNAL );

   if( rc > 0 )
   {
      cbClass->onMessageReceived( sockNumber, tmp_buffer, rc );   // Notify the reception of the message
   }

   return rc;
}

// Activate acception of new connection requests
void CServerSocket::activateAccept()
{
   acceptActive = true;
}

// Dectivate acception of new connection requests
void CServerSocket::deactivateAccept()
{
   acceptActive = false;
}

// Send a block of bytes to a single socket
int CServerSocket::sendBytes( const void* data, size_t len, int flags, int sockno )
{
   int rc = 0;

   if( len == 0 ) return 0;

   rc = send( sockno, data, len, flags );
#ifdef SOCKDBG
CTrace::hexdmp( (unsigned char*)data, len );
#endif

   // If error, remove connection:
   if( rc <= 0 )
   {
//XXX throw excep
       removeConnection( sockno );
#ifdef SOCKDBG
      std::stringstream ss;
      ss << "-- SEND FAIL " << len << \
            " BYTES [have " << connectedSockets.size() << " connections]" << endl;
      CTrace( cout, TRACE_ERROR ) << ss.str() << endl;
#endif
   }

   return rc;
}

// Opening of connection
void CServerSocket::openedSocket( int sockno )
{
   connectedSockets.push_back( sockno );
}

// Remove a closed socket from the list. Make sure it is really closed.
void CServerSocket::removeConnection( int sockno )
{
   std::deque< int >::iterator fit = \
         std::find( connectedSockets.begin(), connectedSockets.end(), sockno );

   if( fit != connectedSockets.end() )
   {
      CTrace( cout, TRACE_PROCESS ) << \
         "-- CLIENT CONNECTION CLOSED [conn " << *fit << "]" << endl;

      close( sockno );

      // Notify the application
      cbClass->onConnectionClose( *fit );

      // Erase this connection from the list of current client connections
      connectedSockets.erase( fit );
   }
}

// Remove all connected sockets:
void CServerSocket::removeAllConnections( )
{
   std::deque<int>::iterator fit = connectedSockets.begin();
   while( fit != connectedSockets.end() )
   {
      CTrace( cout, TRACE_PROCESS ) << \
         "-- CLIENT CONNECTION CLOSED [conn " << *fit << "]" << endl;

      close( *fit );

      // Notify the application
      cbClass->onConnectionClose( *fit );

      // Remove the connection from the list
      fit = connectedSockets.erase( fit );
   }
}

// Returns a list of currently connected sockets (intended for trace)
const std::string CServerSocket::getConnectedSocketList( )
{
   std::stringstream ss;
   std::deque<int>::iterator fit = connectedSockets.begin();

   bool first(true);

   while( fit != connectedSockets.end() )
   {
      if( first == false )
          ss << ",";
      else
         first = false;

      ss << *fit;

      fit++;
   }

   return ss.str();
}

