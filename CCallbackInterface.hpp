#ifndef __CALLBACK_INTERFACE_HPP__
#define __CALLBACK_INTERFACE_HPP__

#include <sys/types.h>

class CTimer;

// Pure virtual class defining an interface for callback functions
class CCallbackInterface {
public:
   virtual void onMessageReceived( int cliSockOrigin, u_int8_t* buf, unsigned int len ) = 0;
   virtual void onTimerExpire( CTimer& timer ) = 0;
   virtual void onNewConnection( int clientSocket ) = 0;
   virtual void onConnectionClose( int clientSocket ) = 0;
};

#endif
