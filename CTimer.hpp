#ifndef __TIMERCPP__
#define __TIMERCPP__

#include <pthread.h>
#include <unistd.h>

#include <time.h>

#include "CCallbackInterface.hpp"

// Object timer - runs a private thread with a sleep and warns on expire
class CTimer
{
public:
    CTimer( int clisock, unsigned int millis, int ident, CCallbackInterface* cbDoor, int type=T1_TIMER );
    ~CTimer( );

    void startTimer( );
    void cancelTimer( );

    bool isRunning();

    int getId();
    int getType();
    int getSocket();

    // Timer types
    static const int T1_TIMER=1; // T1 timer after sending I message
    static const int T2_TIMER=2; // T2 timer after recv I message for the first time
    static const int T3_TIMER=3; // T3 timer after recv S message to send a TESTFRACT

private:
    static void* run( void* ptr );
    void updateThreadState();

    int rc;            // Result of thread creation
    pthread_t thread;  // Thread identifier
    int id;            // Identifier associated to timer
    int type;          // Identifier associated to timer
    unsigned int sg;   // Seconds to wait
    int clientSocket;  // Socket of client connection that message for which the timer is active
                       // was received from or sent to

    bool endOfTimer;   // To control execution of timer

    // Pointer to callback function
    CCallbackInterface* cbClass;

    // Intercept USR1 signal to interrupt sleep call
    static void sigusr1_handler( int signum );
};

#endif
