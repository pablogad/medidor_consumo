#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <pcrecpp.h>

#include <sqlite_db.h>
#include <envi_raw_msg.h>

#include <timing.h>
#include <pt6312.h>

#include "CServerSocket.hpp"
#include "CMessage.hpp"

#define PUERTO_ENVI_R "/dev/ttyUSB0"
#define SIZE_BUF 32768

volatile sig_atomic_t term = 0;

void sigterm_handler( int signum )
{
   term = 1;
}


using namespace std;

class applicVFD : public CCallbackInterface
{
private:
    CServerSocket *sSock;
    envi_raw_msg* m;

public:
    applicVFD() : sSock( NULL ), m( NULL ) {}

    void run()
    {
        string buf;
        sqlite_db *theDB = NULL;
        int rc_db;
        int rc;
        char *db_err_msg=NULL;

        /* Display data */
        u_int8_t data[ 12 ];

        /* Initialize display */
        memset( data, 0, 12 );
        rc = init_pt6312( data );

        setUpDataArea( data );
        setUpTimer( 250 );
        setStringLeft( data, (u_int8_t*)"----" );

        try
        {
           theDB = new sqlite_db( "peibls.db" );

           theDB->query_db( \
                "CREATE TABLE IF NOT EXISTS sensores (id integer not null," \
                                                     "type integer not null," \
                                                     "n_ch integer," \
                "PRIMARY KEY( id,type ));" );
    
           theDB->query_db( \
                "CREATE TABLE IF NOT EXISTS log (timestamp integer not null PRIMARY KEY," \
                                                "sensor_id integer not null," \
                                                "sensor_type integer not null," \
                                                "temp real," \
                "FOREIGN KEY( sensor_id,sensor_type ) REFERENCES sensores( id,type ));" );
           theDB->query_db( \
                "CREATE TABLE IF NOT EXISTS log_canal (timestamp integer not null PRIMARY KEY," \
                                                      "sensor_id integer not null," \
                                                      "sensor_type integer not null," \
                                                      "num_canal integer," \
                                                      "watts integer not null," \
                "FOREIGN KEY( timestamp ) REFERENCES log( timestamp )," \
                "FOREIGN KEY( sensor_id,sensor_type ) REFERENCES sensores( id,type ));" );
        }
        catch( sqlite_excp )
        {
            delete theDB;
            exit(-1);
        }

        // Cargar config de sensores de BD:
//...

        // Crear socket de escucha por el puerto 6666
        sSock = new CServerSocket(6666, this);

        // Leer del dispositivo:

#ifdef EN_CPLUS
        ifstream envir;
        envir.open( PUERTO_ENVI_R, ifstream::in );

        if( envir.is_open() )
        {
            while( envir.good() )
            {
cout << "Esperando msg..." << endl;
               getline( envir, buf );
               extrae_msg( buf, theDB );
//cout << "Buffer tras extraer [" << buf << "]" << endl;
            }

            envir.close();
        }
#else
        int fd;

        if( ( fd = open( PUERTO_ENVI_R, O_RDWR|O_NOCTTY ) ) >= 0 )
        {
            struct termios tio;
            char buffer[ SIZE_BUF+1 ];
            int rc;
            int nbytes;
            fd_set fs;

            /* Obtener info termios: */
            rc = tcgetattr( fd, &tio );

            /* Establecer 57600 baudios, etc: */
            if( rc == 0 )
            {
              tio.c_cflag = B57600 | CS8 | CLOCAL | CREAD;
              tio.c_iflag = IGNPAR;
              tio.c_lflag = ~ICANON;
              tio.c_cc[VTIME] = 0;
              tio.c_cc[VMIN]  = 0;
	      tcflush(fd, TCIFLUSH);
            }
            if( rc == 0 )
               rc = tcsetattr( fd, TCSANOW, &tio );
            if( rc != 0 )
               perror( "ERROR al cambiar velocidad de puerto serie" );

            sSock->activateAccept();

            struct sigaction action;
            memset(&action, 0, sizeof(struct sigaction));
            action.sa_handler = sigterm_handler;
            sigaction( SIGTERM, &action, NULL );

            // Leer forever (salvo fin de prog):
            u_int8_t gridsCirc = 1;
            time_t last_ck = time( NULL );

            while( rc != -1 && term == 0 )
            {
               struct timeval tv = {4L,0L};
               time_t last_read = time( NULL );

               FD_ZERO( &fs );
               FD_SET( fd, &fs );

               // Reiniciar timer al llegar a 0
               if( tv.tv_sec == 0L ) {
                  tv.tv_sec = 4L;
               }
               rc = select( fd+1, &fs, NULL, NULL, &tv );    // Esperar a ke haya algo

               if( rc != -1 )
               {
                  if( FD_ISSET( fd, &fs ) )
                  {  nbytes = (int)read( fd, buffer, SIZE_BUF );
                     if( nbytes != -1 )
                     {
                        buf.append( buffer, nbytes );
                        if( envi_raw_msg::envi_msg_completo( buf ) == true )
                        {
                           if( m == NULL ) m = new envi_raw_msg( buf, theDB, data, sSock );
                           else m->update_msg( buf );

                           m->procesa_msgs();

                           buf.clear();
                        }
                     }

                     //if( time_last_ck - time_last_read > THRESHOLD_RECORD_FILE )
                     //{ time_last_read = time_last_ck;
                     //  select * from log_canal order by timestamp limit 100;  UNA HORA
                     //}
                  }
                  else
                  {  // El tiempo ha pasado sin eventos
                  }
               }
               else if( errno == EINTR )
               {
                  time_t rawtime;
                  struct tm* loct;
                  char tmstr[ 8 ];
    
                  time( &rawtime );
                  loct = localtime( &rawtime );
                  strftime( tmstr, sizeof( tmstr ), " %H%M", loct );

                  // Print on display data
                  setStringRight( data, (u_int8_t*)tmstr );
                  // comentado para flasheo : setDotsRight( data, RIGHT_DOTS );

                  rc = 0;  // No error
               }

               if( rc == 0 ) {
                  // Pintar circulillo por segundos
                  if( last_read - last_ck > 0 ) {
                     // Inc Grids
                     // data[7] = gridsCirc;
                     /* Completar circulo 
                     gridsCirc = gridsCirc * 2 + 1;
                     if( gridsCirc >= 0x7F ) gridsCirc = 0; */
                     /* Circulo progreso 
                     gridsCirc = gridsCirc * 2;
                     if( gridsCirc >= 0x20 ) gridsCirc = 1; */
                     last_ck = time( NULL );
                     /* Flashear : de la hora cada segundo */
                     data[8] ^= 0x80;
                  }
               }

               if( rc == 0 ) { sSock->manage(); }
            }
        }
        else
        {
            perror( "ERROR al abrir el puerto" );
        }

#endif

        delete sSock; sSock = NULL;

        setStringLeft( data, (u_int8_t*)"----" );
        setStringRight( data, (u_int8_t*)"    " );

        close_pt6312();
    }

    void onMessageReceived(int cliSock, u_int8_t* data, unsigned int length)
    { 
        std::cout << "-> RECIBIDO MENSAJE DESDE CONEXION " << cliSock << ", LEN " << length << std::endl;

        CMessage* msg = new CMessage( string((char*)data) );

        if( msg->getType() == CMessage::MSG_REQ ) {
           string resp;
           msg->getResponse( resp, m );
           char* cResp = (char*)resp.c_str();

           sSock->sendBytes( cResp, strlen(cResp), MSG_NOSIGNAL, cliSock );
        }
        else if( msg->getType() == CMessage::MSG_CMD_LIVEDATA ) {
           // Enviar datos a cliente hasta nueva orden
           m->liveData = true;
           m->liveDataFd = cliSock;
        }
        else if( msg->getType() == CMessage::MSG_CMD_ENDLIVEDATA ) {
           // No enviar datos a cliente
           m->liveData = false;
           m->liveDataFd = -1;
        }
    }

    void onTimerExpire(CTimer&)
    {}

    void onNewConnection(int cliSock)
    {
        std::cout << "-> NUEVA CONEXION " << cliSock << std::endl;
    }
    void onConnectionClose(int cliSock)
    {
        std::cout << "-> CONEXION CERRADA " << cliSock << std::endl;
        m->liveData = false;
        m->liveDataFd = -1;
    }
};

int main (int argc, char *argv[] )
{
    applicVFD app;
    app.run();
}

