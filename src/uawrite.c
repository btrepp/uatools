#include <stdio.h>
#include <stdlib.h>
#include "open62541.h"
void printUsage(int argc, char **argv){
        fprintf(stderr,"Usage: %s namespace tag value\n", argv[0]);
        fprintf(stderr,"\t %s ns=namespace;s=tag value\n", argv[0]);
        char* called =argv[0];
        for(int i=1;i<argc;i++){
            char * current = argv[i];
            char* dest = (char*)malloc(sizeof(char)*(strlen(called)+strlen(current)+2));
            strcpy(dest,called);
            dest=strcat(dest," ");
            called=strcat(dest,current);
        }
        fprintf(stderr,"\t %s\n ",called);
}

int main(int argc, char** argv) {
    const char* default_url = "opc.tcp://127.0.0.1:49380";
    const char* url = getenv("OPCUA_SERVER");
    url = url ? url: default_url;

    UA_NodeId node=UA_NODEID_NULL;
    char* valuestring;

    if(argc==4) { 
        char* nodestring=argv[1];
        char* tagstring=argv[2];
        valuestring= argv[3];

        //user has supplied opc node information to browse
        char* endptr;
        int ns = (int) strtol(nodestring,&endptr,10);

        if(*endptr!='\0'){
            fprintf(stderr,"%s is not a valid namespace id\n",nodestring);
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
    }

    if(argc==3){
        char* combinedstring = argv[1];
        valuestring= argv[2];
        char* tagstringbuffer = (char*) malloc(strlen(combinedstring)*sizeof(char));
        int ns;
        int conversions=sscanf(combinedstring,"ns=%d;s=%s",&ns,tagstringbuffer);

        if(conversions==2){
            node = UA_NODEID_STRING(ns,tagstringbuffer);
        }
    }

    if(UA_NodeId_isNull(&node)){
        printUsage(argc,argv);
        return -1;
    }

    if(valuestring==NULL){
        fprintf(stderr, "BUG, value string was not set");
        return -1;
    }

    UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout);
    UA_StatusCode retval = UA_Client_connect(client, UA_ClientConnectionTCP,url);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }

    //Read current value to determine datatype
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead =  UA_Array_new(1, &UA_TYPES[UA_TYPES_READVALUEID]);
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = node;
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

    UA_ReadResponse rResp = UA_Client_Service_read(client, rReq);
    const UA_DataType* type;
    if(rResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
            rResp.resultsSize > 0 && rResp.results[0].hasValue)
    {
        type = &(*rResp.results[0].value.type);
    }

    if(type==NULL){
        fprintf(stderr,"Unable to determine tag type\n");
        return -1;
    }

    void* data;
    int typeid = type->typeIndex;
    switch(typeid){
        case UA_TYPES_UINT16:
        case UA_TYPES_INT32:{
                                char* endptr;
                                int valueasint= (int) strtol(valuestring, &endptr,10);
                                if(*endptr=='\0'){
                                    data= &valueasint;
                                }
                                else{
                                    fprintf(stderr,"Input is not int: %s\n",valuestring);
                                    return -1;
                                }
                                break;
                            }
        case UA_TYPES_STRING:{
                                 UA_String string = UA_String_fromChars(valuestring);
                                 data= &string;
                                 break;
                             }
    }

    if(data==NULL){
        fprintf(stderr,"Error casting type for %s. OPCTypeId:%d, Check OPC Server type matches\n",valuestring,typeid);
        return -1;
    }


    //Write value
    UA_WriteRequest wReq;
    UA_WriteRequest_init(&wReq);
    wReq.nodesToWrite = UA_WriteValue_new();
    wReq.nodesToWriteSize = 1;
    wReq.nodesToWrite[0].nodeId = node;
    wReq.nodesToWrite[0].attributeId = UA_ATTRIBUTEID_VALUE;
    wReq.nodesToWrite[0].value.hasValue = true;
    wReq.nodesToWrite[0].value.value.type = type;
    wReq.nodesToWrite[0].value.value.storageType = UA_VARIANT_DATA_NODELETE; //do not free the integer on deletion
    wReq.nodesToWrite[0].value.value.data = data;

    UA_WriteResponse wResp = UA_Client_Service_write(client, wReq);
    //UA_WriteRequest_deleteMembers(&wReq);
    UA_WriteResponse_deleteMembers(&wResp);
    UA_Client_disconnect(client);
    UA_Client_delete(client);

    return wResp.responseHeader.serviceResult;

}
