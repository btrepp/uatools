#include <stdio.h>

#include "open62541.h"

int main(int argc, char** argv) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout);
    UA_StatusCode retval = UA_Client_connect(client, UA_ClientConnectionTCP,argv[1]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}
