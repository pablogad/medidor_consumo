#include "CMessage.hpp"
#include <iostream>

using std::cout;
using std::endl;

CMessage::CMessage( const string& data ) : lastcmd( CMD_NONE )
{
    // FMT: <CMD> <OBJ> <ARGS> (CMD command operates on OBJect)

    // Parsear el comando
    int pos_spc = data.find_first_of( "\r\n", 0 );
    int ini_pos = 0;

    if( pos_spc != string::npos )
    {
        msg = data.substr( 0, pos_spc );  // Remove \r\n
        pos_spc = msg.find( ' ' );
        cmd = msg.substr( 0, pos_spc );
        ini_pos = pos_spc+1;
    }

    if( pos_spc != string::npos )
    {
        pos_spc = msg.find( ' ', ini_pos );
        if( pos_spc != string::npos ) {
           obj = msg.substr( ini_pos, pos_spc-ini_pos );
           ini_pos = pos_spc+1;
           args = msg.substr( ini_pos );
        }
        else {
           obj = msg.substr( ini_pos );
           pos_spc = 0;  // Avoid abort
        }
    }

    cout << "-> MSG[" << msg << "], cmd[" << cmd << "], obj[" << obj << "], args[" << args << "]" << endl;

    lastcmd = -1;

    // Apply command
    if( pos_spc != string::npos )
    {
        // DISP: display
        if( cmd == "DISP" )
        {
            if( obj == "LEFT" )
            {
            }
            else if( obj == "RIGHT" )
            {
            }
        }

        // GET: leer dato
        else if( cmd == "GET" ) {
           if( obj == "LAST" ) {  // Última medida
              lastcmd = CMD_GETLAST;
              cout << "== COMANDO: GET LAST" << endl;
           }
        }
        else if( cmd == "CAPTURE" && obj == "DATA" ) {
           if( args == "START" ) {
              lastcmd = CMD_CAPTURE_START;
              cout << "== COMANDO: START CAP" << endl;
           }
           else if( args == "STOP" ) {
              lastcmd = CMD_CAPTURE_STOP;
              cout << "== COMANDO: STOP CAP" << endl;
           }
        }
    }
}

// Get response from a received message
void CMessage::getResponse( string& resp, envi_raw_msg* ref )
{
   resp.clear();

   if( ref ) {
   if( lastcmd == CMD_GETLAST ) {
      if( args == "" ) {
         // Resp: last read data
         ref->getLastRead( resp );
      }
      else {
         // Pattern: XXh - last XX hours 00-99
         const char* pstr = args.c_str();
         if( isdigit( pstr[0] ) && isdigit( pstr[1] ) && pstr[2] == 'h' ) {
            // SELECT DB
         }
      }
   }
   else if( lastcmd == CMD_CAPTURE_START ) {
      resp = "OK ACT";
   }
   else if( lastcmd == CMD_CAPTURE_STOP ) {
      resp = "OK DEACT";
   }
   else
      resp = "NOK";
   }

   cout << "   Response [" << resp << "]" << endl;
}

int CMessage::getType()
{
   if( lastcmd == CMD_GETLAST )            return MSG_REQ;
   else if( lastcmd == CMD_CAPTURE_START ) return MSG_CMD_LIVEDATA;
   else if( lastcmd == CMD_CAPTURE_STOP )  return MSG_CMD_ENDLIVEDATA;
}
