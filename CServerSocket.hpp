#ifndef __CSERVER_SOCKET_HPP__
#define __CSERVER_SOCKET_HPP__

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <deque>
#include <queue>
#include <vector>

#include "CCallbackInterface.hpp"


// Class containing a server management object for connections
class CServerSocket {

public:
   CServerSocket( int port, CCallbackInterface* cbDoor );
   ~CServerSocket();

   void manage();
   void activateAccept();
   void deactivateAccept();
   inline bool isActivatedAccept() { return acceptActive; }

   inline unsigned int getConnectedSocketCount() { return connectedSockets.size(); }
   const std::string getConnectedSocketList();

   void removeConnection( int sockno );
   void removeAllConnections();

   int readData( int );
   int sendBytes( const void* data, size_t len, int flags, int sockno );

   void openedSocket( int );

private:
   // Managed sockets:
   int listenSocket;
   std::deque< int > connectedSockets;

   // Internal variables:
   bool acceptActive;

   void __accept_internal();

   // Pointer to class with callback functions:
   CCallbackInterface* cbClass;
};

#endif

