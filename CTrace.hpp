#ifndef __CTRACE__
#define __CTRACE__

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <time.h>

// Utility functions
inline std::ostream& timestamp( std::ostream& out )
{
   static char curTime[ 128 ];
   struct timespec ts;
   clock_gettime( CLOCK_REALTIME, &ts );
   ctime_r( &ts.tv_sec, curTime );
   curTime[ 19 ] = '\0';
   return out << &curTime[11] << "." << ts.tv_nsec;
   
#if 0
   std::time_t t = std::time( NULL );
   std::tm tm = *std::localtime(&t);
   std::strftime( (char*)curTime, 20, "%d-%m-%Y %H:%M:%S", &tm );
   return out << curTime;
#endif
}

enum traceLevels { TRACE_NONE=0, TRACE_ERROR, TRACE_PROCESS, TRACE_VERBOSE, TRACE_ULTRA, TRACE_ENDMARKER };

class CTrace {

public:
   CTrace( std::ostream&, int lvl=255 );
   ~CTrace() { if( tracelevel <= global_tracelevel ) kflush(); }

   void kflush();

   // Operator to fill the buffer with several datatypes:
   template <class T>
   CTrace& operator<<( const T& typ )
   {
      if( tracelevel <= global_tracelevel )
         ss << typ;

      return *this;
   }

   CTrace& operator<<( std::ostream& (*f)( std::ostream& ) )
   {
      if( tracelevel <= global_tracelevel )
         f( ss );

      return *this;
   }

   // Return a copy of held text:
   std::string getText()
   {
      return ss.str();
   }

   // Static functions - to use 'objectless'
   static void hexdmp( unsigned char* ptr, int sze );
   static void setGlobalTraceLevel( int );
   static int getGlobalTraceLevel( );

private:

   std::ostream& buffer;
   std::stringstream ss;

   int tracelevel;
   static int global_tracelevel;
};

#endif

