#include <sys/types.h>
#include <string>

#include "envi_raw_msg.h"

using std::string;


class CMessage {
private:
    string msg;
    string cmd;  // Received command
    string obj;  // Object used by the command
    string args; // Arguments

    enum cmd { CMD_NONE=0, CMD_GETLAST, CMD_DISPLEFT, CMD_DISPRIGHT, CMD_CAPTURE_START, CMD_CAPTURE_STOP };

    int lastcmd;

public:
    CMessage( const string& data );

    void getResponse( string& resp, envi_raw_msg* ref );
    int getType();

    enum { MSG_REQ=1, MSG_CMD_LIVEDATA, MSG_CMD_ENDLIVEDATA };
};
