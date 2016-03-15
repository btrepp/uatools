#include <stdio.h>
#include <stdlib.h>

#include "open62541.h"

int main(int argc, char** argv) {
    const char* default_url = "opc.tcp://127.0.0.1:49380";
    const char* url = getenv("OPCUA_SERVER");
    url = url ? url: default_url;

    UA_NodeId node=UA_NODEID_NULL;
    if(argc==3) { 
        //user has supplied opc node information to browse
        char* endptr;
        int ns = (int) strtol(argv[1],&endptr,10);

        if(argv[1]==endptr){
            printf("%s is not a valid namespace id\n",argv[1]);
            return -1;
        }

        int nodeid = (int) strtol(argv[2], &endptr,10);
        if(argv[1]!=endptr){
            //Wasn't an integer, so assume string id
            node =UA_NODEID_STRING(ns,argv[2]);
        }else{
            //It was a number so use numeric
            node =UA_NODEID_NUMERIC(ns, nodeid);
        }
    }
    if(UA_NodeId_isNull(&node)){
        printf("Usage %s namespace tag", argv[0]);
        return -1;
    }

    UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout);
    UA_StatusCode retval = UA_Client_connect(client, UA_ClientConnectionTCP,url);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }

    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead =  UA_Array_new(1, &UA_TYPES[UA_TYPES_READVALUEID]);
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = node;
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

    UA_ReadResponse rResp = UA_Client_Service_read(client, rReq);
    if(rResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
            rResp.resultsSize > 0 && rResp.results[0].hasValue)
    {
        int value = *(UA_Int32*)rResp.results[0].value.data;
        printf("the value is: %i\n", value);
    }


    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}
