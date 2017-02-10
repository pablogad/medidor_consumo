#ifndef __ENVI_RAW_MSG_H__
#define __ENVI_RAW_MSG_H__

#include <string>
#include <pcrecpp.h>
#include <sys/types.h>

#include <sqlite_db.h>
#include <CServerSocket.hpp>

// Pa guardar dato
typedef struct {
      int ch;
      int watts;
      float temp;
} datos_msg;

// Rutinas para tratar el mensaje que llega desde el dispositivo.
// Crea objetos envi_msg y graba a BD:

using namespace pcrecpp;

class envi_raw_msg {
public:
   envi_raw_msg( const string& raw_msg, sqlite_db* db, u_int8_t* data, CServerSocket* cs );
   ~envi_raw_msg( );

   void update_msg( const string& );
   void procesa_msgs( void );

   static inline bool envi_msg_completo( const string& msg )
   {
      if( msg.find( "<msg>" ) != string::npos &&
          msg.find( "</msg>" ) != string::npos ) return true;
      else return false;
   }

   // Solicitudes cliente
   void getLastRead( string& m );
   void getSomeReads( string& s, const time_t ini, const time_t fin );

   bool liveData;
   int liveDataFd;  // Envio de datos continuo a traves de socket FD


private:

   enum { MSG_LIVE, MSG_HIST, MSG_UNKNWN };

   void msg( const string& );
   void deco_live_msg( const string& );
   void deco_hist_msg( const string& );

   // Utilidades:
   static inline int envi_tipo_msg( const string& msg )
   {
      RE re1( string( "<hist>.*?</hist>" ) );
      RE re2( string( "<sensor>\\d+</sensor><id>\\d+</id>" ) );
      if( re1.PartialMatch( msg ) ) return MSG_HIST;
      if( re2.PartialMatch( msg ) ) return MSG_LIVE;
      else                          return MSG_UNKNWN;
   }

   string theMsg;
   sqlite_db* theDB;
   u_int8_t* displayData;

   CServerSocket* sSock;

   // Estructura con ultimo mensaje
   datos_msg lastmsg;
};

#endif  //__ENVI_RAW_MSG_H__

