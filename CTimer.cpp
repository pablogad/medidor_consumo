#include <stdexcept>
#include <iostream>

#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include "CTimer.hpp"
#include "CTrace.hpp"

using std::cout;
using std::dec;
using std::hex;
using std::endl;

// Timer class implementation
CTimer::CTimer( int clisock, unsigned int time, int ident, \
            CCallbackInterface* cbDoor, int timerType ) : \
            cbClass( cbDoor ), clientSocket( clisock ), \
            sg( time ), id( ident ), type( timerType ), \
            thread( 0 ), endOfTimer( false )
{
CTrace( cout, TRACE_ULTRA ) << "CTOR CTIMER!!" << hex << this << dec << " " << timestamp << endl;
   // Set up sigusr1 signal if not done
   struct sigaction action, oldAction;

   sigaction( SIGUSR1, NULL, &oldAction );
   if( oldAction.sa_handler != CTimer::sigusr1_handler )
   {
      memset( &action, 0, sizeof( struct sigaction ) );
      action.sa_handler = CTimer::sigusr1_handler;
      action.sa_flags = 0;
      sigaction( SIGUSR1, &action, NULL );
   }
}

// Dtor
CTimer::~CTimer()
{
CTrace( cout, TRACE_ULTRA ) << "DTOR CTIMER id=" << id << ",type="<< type << "!!" << hex << this << dec << " " << timestamp << endl;
   if( thread != 0 )
   {
      cancelTimer();
      pthread_join( thread, NULL );
   }
}

// Interruption handler
void CTimer::sigusr1_handler( int signum )
{
   // (does nothing)
}

// Run a new thread
void CTimer::startTimer()
{
   updateThreadState();

   if( thread != 0 )
   {
      CTrace( cout, TRACE_ERROR ) << \
          "*ERROR* thread for timer T" << type << " timer " << id << \
          " is already running!" << endl;
      return;
   }

   // Run the new thread
   endOfTimer = true;
   if( rc = pthread_create( &thread, NULL, &CTimer::run, (void*)this ) )
   {
      throw std::runtime_error( strerror( errno ) );
   }

   // Wait until it is really running before returning
   int cnt=0;
   while( endOfTimer == true && cnt++ < 500 ) usleep( 1 );
}

// Thread function:
void *CTimer::run( void* ptr )
{
   // Copy relevant information to local variables to prevent failure
   // if caller object is deleted:
   CTimer* callerObj = (CTimer*)ptr;
   unsigned int remain = callerObj->sg;
   int id = callerObj->id;

   time_t initialTime = time( NULL );
   time_t ellapsedTime = 0;

   CTrace( cout, TRACE_PROCESS ) << \
                    "-- Start timer T" << callerObj->type << " id " << id << \
                    " (" << remain << " sg) @ " << timestamp << endl;

   callerObj->endOfTimer = false;

   while( remain = sleep( remain ) > 0 && callerObj->endOfTimer == false && \
               ( ellapsedTime = time( NULL ) - initialTime ) < callerObj->sg );

   if( callerObj->endOfTimer == true )
   {
      CTrace( cout, TRACE_PROCESS ) << \
            "-- Timer T" << callerObj->type << " id " << id << \
            " interrupted, " << callerObj->sg-time(NULL)+initialTime << \
            " sg. left @ " << timestamp << endl;
   }
   else
   {
      CTrace( cout, TRACE_PROCESS ) << \
            "-- Time T" << callerObj->type << " id " << id << \
            " expired @ " << timestamp << endl;
      callerObj->cbClass->onTimerExpire( *callerObj );
   }

   // Exit of thread
}

// Check if the thread has died
void CTimer::updateThreadState( )
{
   if( thread )
      if( pthread_kill( thread, 0 ) != 0 )
          thread = 0;  // NOT running!
}

// Test if the thread is currently running
bool CTimer::isRunning( )
{
   updateThreadState();
   if( thread == 0 )
   {
      return false;
   }
   else
   {
      return true;
   }
}

// Cancel a running timer
void CTimer::cancelTimer( )
{
   updateThreadState();

   endOfTimer = true;

   if( !thread ) return;

   // Send a signal to the thread to interrupt the sleep call:
   pthread_kill( thread, SIGUSR1 );
}

// Get the id of this timer
int CTimer::getId( )
{
   return id;
}

// Get the type of this timer
int CTimer::getType( )
{
   return type;
}

// Get the socket of the connection
int CTimer::getSocket( )
{
   return clientSocket;
}

