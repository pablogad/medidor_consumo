#ifndef __CCLIENT_SOCKET_HPP__
#define __CCLIENT_SOCKET_HPP__

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <queue>
#include <string>

#include "CCallbackInterface.hpp"


// Class containing a client management object for IEC104 protocol
class CClientSocket {

public:
   CClientSocket( const std::string& server, int port, CCallbackInterface* cbDoor );
   ~CClientSocket();

   int getSocket();

   void manage();

   virtual int readData( );
   int sendBytes( const void* data, size_t len, int flags );

   void closeConnection();

private:

   int clientSocket;

   // Pointer to class with callback functions:
   CCallbackInterface* cbClass;
};

#endif

