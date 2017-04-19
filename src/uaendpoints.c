#include <stdio.h>
#include <stdlib.h>
#include "open62541.h"

int main(int argc, char** argv) {
    const char* default_url = "opc.tcp://127.0.0.1:49380";
    const char* url = getenv("OPCUA_SERVER");
    url = url ? url: default_url;
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);

    //listing endpoints
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval =
        UA_Client_getEndpoints(client, url,
                &endpointArraySize, &endpointArray);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }

    printf("%i endpoints found\n", (int)endpointArraySize);
    for(size_t i=0;i<endpointArraySize;i++){
        printf("URL of endpoint %i is %.*s\n", (int)i, (int)endpointArray[i].endpointUrl.length, endpointArray[i].endpointUrl.data);
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}
