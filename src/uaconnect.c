#include <stdio.h>
#include <stdlib.h>

#include "open62541.h"

int main(int argc, char** argv) {
    const char* default_url = "opc.tcp://127.0.0.1:49380";
    const char* url = getenv("OPCUA_SERVER");
    url = url ? url: default_url;
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, url);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}
