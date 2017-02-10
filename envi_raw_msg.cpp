#include <string>

//TEMP
#include <iostream>
#include <sstream>
using namespace std;
// \TEMP

#include <envi_raw_msg.h>
#include <pt6312.h>

// Rutinas para tratar el mensaje que llega desde el dispositivo. Puede haber uno o mas
// bloques <msg> </msg>

// Decodifica el mensaje y graba datos a BD:
envi_raw_msg::envi_raw_msg( const string& raw_msg, sqlite_db* db, u_int8_t* data, CServerSocket* cs ) :
      theMsg( raw_msg ), theDB( db ), displayData( data ),
      liveData( false ), liveDataFd( -1 ), sSock( cs )

{
#ifdef DEBUG_ENVI
   cout << "envi_raw_msg::" << theMsg << endl;
#endif
};

envi_raw_msg::~envi_raw_msg() {
}

// procesa_msgs: busca bloques <msg> y los separa y procesa cada uno de ellos:
void envi_raw_msg::procesa_msgs( void )
{
   string bloque_msg;

   RE re( "<msg>(.*?)</msg>", RE_Options().set_multiline(true) );
   while( re.PartialMatch( theMsg, &bloque_msg ) )
   {
      // Quitar de buf el mensaje:
      int pos = theMsg.find( "<msg>" + bloque_msg );
      if( pos != string::npos )
      {
         theMsg.erase( pos, bloque_msg.length() + 11 );  // se añaden longitudes de etiq msg
         msg( bloque_msg );
      }
   }
}

// Actualizar el msg solo si nos llega uno completo:
void envi_raw_msg::update_msg( const string& msg )
{
   if( envi_msg_completo( msg ) == true )
   {
#ifdef DEBUG_ENVI
cout << "envi_raw_msg::update_msg:theMsg vale (" << theMsg << ") msg:" << msg << endl;
#endif
      theMsg = msg;
   }
}

void envi_raw_msg::msg( const string& msg )
{
   // Extraer cabecera común:
   string header_re( "<src>.+?</src>(<uid>(.+?)</uid>|)<dsb>([0-9]+)</dsb>" \
                     "(<time>([0-9][0-9]:[0-9][0-9]:[0-9][0-9])</time>|)(.*)" );
   string uidf,uid,timef,time;
   int dsb;
   int tipo_msg;
   string cuerpo_msg;

   RE re( header_re, RE_Options().set_multiline( true ) );
   if( re.PartialMatch( msg, &uidf, &uid, &dsb, &timef, &time, &cuerpo_msg ) )
   {
      if( ( tipo_msg = envi_tipo_msg( msg ) ) == MSG_LIVE )
      {
         deco_live_msg( cuerpo_msg );
      }
      else if( tipo_msg == MSG_HIST )
      {
         deco_hist_msg( cuerpo_msg );
      }
      // else throw ....
   }
}


// Decodificar live msg:
void envi_raw_msg::deco_live_msg( const string& msg )
{
   string datos_comunes_re( "(<tmpr>([\\.0-9]+)</tmpr>|)" \
                            "(<tmprF>([\\.0-9]+)</tmprF>|)" \
                            "<sensor>(\\d+)</sensor>" \
                            "<id>(\\d+)</id>" \
                            "<type>(\\d+)</type>(.*)" );
   string msg_data;
   string kktmpc,kktmpf;
   string tmpc, tmpf;
   int sens, id, type;

   RE re( datos_comunes_re, RE_Options().set_multiline( true ) );
   if( re.PartialMatch( msg, &kktmpc, &tmpc, &kktmpf, &tmpf, &sens, &id, &type, &msg_data ) )
   {
       static char horayfecha[128];
       time_t rawtime;
       struct tm* timeinfo;

       time( &rawtime);
       timeinfo = localtime( &rawtime );
       strftime( horayfecha, 128, "%c", timeinfo );

       cout << horayfecha << ": TEMP:" << tmpc << ", SENS:" << sens << ", ID:" << id << ", TYPE:" << type << endl;

       // Si el sensor no existe insertarlo:
       //INSERT OR IGNORE INTO sensores ( ID, TYPE, N_CH ) VALUES( id, type, 0 );
       vector<string> cols;
       obj_db<int,int,int> el;
       null_db nulls;
       cols.push_back( "ID" );
       cols.push_back( "TYPE" );
       cols.push_back( "N_CH" );
       el.v1 = id;
       el.v2 = type;
       el.v3 = 0;      // En este momento no sabemos el número de canales
       theDB->insert_or_ignore_obj( string( "sensores" ), cols, el, &nulls );

       // Type 1: from CT, 2-4: from impulse meter (elec/gas/water)
       if( type == 1 )
       {
          int max_ch=-1;  // Máximo número de canal que se lee del sensor en caso de tipo 1
          RE re2( "(<ch(\\d)>|)<watts>(\\d+)</watts>(</ch\\2>|)", RE_Options().set_multiline( true ) );
          StringPiece text( msg_data );
          string kkch;
          int ch,watts;
          while( re2.Consume( &text, &kkch, &ch, &watts, &kkch ) )
          {
              cout << " --> CANAL:" << ch << ", WH:" << watts << endl;
//XXXX DISPLAY
if( ch == 1 ) {
   std::stringstream ssw;
   ssw.width( 4 );
   ssw << watts;
   setStringLeft( displayData, (u_int8_t*)ssw.str().c_str() );
}

// XXX lastmsg
lastmsg.ch = ch;
lastmsg.watts = watts;
lastmsg.temp = ::atof( tmpc.c_str() );

// Envio a cliente
if( liveData ) {
   std::stringstream resp;
   resp << time(NULL) << ";" << lastmsg.temp << ";" << lastmsg.watts << ";" << lastmsg.ch << endl;
   const char* cResp = resp.str().c_str();
   sSock->sendBytes( cResp, strlen(cResp), MSG_NOSIGNAL, liveDataFd );
}

              // Insertar en BD: tabla log con temperatura
              cols.clear();
              cols.push_back( "TIMESTAMP" );
              cols.push_back( "SENSOR_ID" );
              cols.push_back( "SENSOR_TYPE" );
              cols.push_back( "TEMP" );
              obj_db<int,int,int,double> el_log;
              el_log.v1 = time(NULL);  // Timestamp
              el_log.v2 = id;
              el_log.v3 = type;
              el_log.v4 = atof( tmpc.c_str() );  // Tipo REAL (double)
              theDB->insert_or_ignore_obj( string( "log" ), cols, el_log, &nulls );

              // Tabla log_canal con watts para cada canal:
              cols.clear();
              cols.push_back( "TIMESTAMP" );
              cols.push_back( "SENSOR_ID" );
              cols.push_back( "SENSOR_TYPE" );
              cols.push_back( "NUM_CANAL" );
              cols.push_back( "WATTS" );
              obj_db<int,int,int,int,int> el_log_canal;
              el_log_canal.v1 = el_log.v1;
              el_log_canal.v2 = id;
              el_log_canal.v3 = type;
              el_log_canal.v4 = ch;
              el_log_canal.v5 = watts;
              theDB->insert_or_ignore_obj( string( "log_canal" ), cols, el_log_canal, &nulls );

              if( ch > max_ch ) max_ch = ch;
          }

           // Actualizar núm canales en el sensor:
          if( max_ch != -1 )
          {  // Columnas:
             obj_db<int> vals_to_set;
             obj_db<int,int> vals_to_where;

             cols.clear();

             // Valores para update:
             vals_to_set.v1 = max_ch;

             // Columnas para update:
             cols.push_back( "N_CH" );

             // Valores para where:
             vals_to_where.v1 = id;
             vals_to_where.v2 = type;

             // Columnas para where:
             cols.push_back( "ID" );
             cols.push_back( "TYPE" );

             theDB->update_obj( string( "sensores" ), vals_to_set, vals_to_where, cols );
          }
       }
       else if( type >= 2 && type <= 4 )  // 2: electricidad, 3: gas, 4: agua
       {
          RE re2( "<imp>(\\d+)</imp><ipu>(\\d+)</ipu>", RE_Options().set_multiline( true ) );
          long imp;
          int ipu;
          if( re2.PartialMatch( msg_data, &imp, &ipu ) )
          {
              cout << " --> IMPULSOS:" << imp << ", IMP/UNIDAD:" << ipu << endl;
               // Insertar en BD
          }
       }
       // else throw
   }
   // else throw no_match
}

// Decodificar hist msg:
void envi_raw_msg::deco_hist_msg( const string& msg )
{
   string datos_comunes_re( "<hist>" \
                            "<dsw>(\\d+)</dsw>" \
                            "<type>(\\d+)</type>" \
                            "<units>(.+?)</units>" \
                            "(.+?)" \
                            "</hist>(.*)" );

   int dsw,type;
   string units;
   string msg_data;

   RE re( datos_comunes_re, RE_Options().set_multiline( true ) );
   if( re.PartialMatch( msg, &dsw, &type, &units, &msg_data ) )
   {
#ifdef DEBUG_ENVI
      cout << " HIST:DSW:" << dsw << ", TYPE:" << type << ", UNITS:" << units << endl;
#endif
      string datos_hdr_re( "<data><sensor>(\\d+)</sensor>(.+?)</data>" );
      string datos_re( "<(m|d|h)(\\d+)>([\\.0-9]+)</\\1\\2>" );

      // Recorrer elementos <data>:
      RE re2( datos_hdr_re, RE_Options().set_multiline( true ) );
      StringPiece text( msg_data );
      int num_sensor;
      string msg_sensor;
      while( re2.Consume( &text, &num_sensor, &msg_sensor ) )
      {
#ifdef DEBUG_ENVI
         cout << " --> DATOS DE SENSOR " << num_sensor << " <--RE=["<< datos_re << "]" << endl;
cout << "       [MSG_SENS:"<<msg_sensor<< "]" << endl;
#endif
         // Recorrer elementos de histórico del sensor:
         RE re3( datos_re, RE_Options().set_multiline( true ) );
         string tipo_acc;
         int ago;
         float accum;
         StringPiece data_piece( msg_sensor );
         while( re3.FindAndConsume( &data_piece, &tipo_acc, &ago, &accum ) )
         {
#ifdef DEBUG_ENVI
            cout << "    > TIPO:" << tipo_acc << ", AGO:" << ago << ", ACCUM:" << accum << endl;
#endif
         }
      }
   }
   // else throw
}

void envi_raw_msg::getLastRead( string& s )
{
   s.clear();

   std::stringstream ss;

   ss << time(NULL) << ";" << lastmsg.temp << ";" << lastmsg.watts << ";" << \
      lastmsg.ch;
   s.append( ss.str().c_str() );
}

void envi_raw_msg::getSomeReads( string& s, const time_t ini, const time_t fin )
{
   s.clear();


//select strftime('%d/%m/%Y %H:%M:%S', datetime(timestamp, 'unixepoch')),watts from (select  * from log_canal order by timestamp desc limit 100) order by timestamp;
}
