#include <stdio.h>
#include <stdlib.h>
#include "open62541.h"

int main(int argc, char** argv) {
    const char* default_url = "opc.tcp://127.0.0.1:49380";
    const char* url = getenv("OPCUA_SERVER");
    url = url ? url: default_url;

    UA_NodeId node=UA_NODEID_NULL;
    UA_Variant *variant = UA_Variant_new();
    if(argc==4) { 
        char* nodestring=argv[1];
        char* tagstring=argv[2];
        char* valuestring= argv[3];

        //user has supplied opc node information to browse
        char* endptr;
        int ns = (int) strtol(nodestring,&endptr,10);

        if(*endptr!='\0'){
            printf("%s is not a valid namespace id\n",nodestring);
            return -1;
        }

        int nodeid = (int) strtol(tagstring, &endptr,10);
        if(*endptr!='\0'){
            //Wasn't an integer, so assume string id
            node =UA_NODEID_STRING(ns,tagstring);
        }else{
            //It was a number so use numeric
            node =UA_NODEID_NUMERIC(ns, nodeid);
        }

        int valueasint= (int) strtol(valuestring, &endptr,10);
        if(*endptr!='\0'){
            UA_String string = UA_String_fromChars(valuestring);
            UA_StatusCode code=UA_Variant_setScalarCopy(variant, &string, &UA_TYPES[UA_TYPES_STRING]);
            if(code!=UA_STATUSCODE_GOOD){
                return code;
            }
        }
        else{
            UA_Int32 value = valueasint;
            UA_StatusCode code=UA_Variant_setScalarCopy(variant,&value, &UA_TYPES[UA_TYPES_INT32]);
            if(code!=UA_STATUSCODE_GOOD){
                return code;
            }
        }
    }
    if(UA_NodeId_isNull(&node)){
        printf("Usage: %s namespace tag value\n", argv[0]);
        return -1;
    }
    if(!UA_Variant_isScalar(variant)){
       printf("Error determining value of %s", argv[3]);
        return -1;
    }


    UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout);
    UA_StatusCode retval = UA_Client_connect(client, UA_ClientConnectionTCP,url);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }

    retval = UA_Client_writeValueAttribute(client, node, variant);
    UA_Variant_delete(variant);

    UA_Client_disconnect(client);
    UA_Client_delete(client);

    return retval;

}
