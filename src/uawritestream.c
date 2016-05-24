#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "open62541.h"

#define READSIZE 1024
#define DELIMITER ','


#define NO_BUFFER -2
#define MISSING_DELIMITER -3
#define EXTRA_DELIMITER -4
#define NOT_INT -5

typedef struct parsedLine parsedLine;
struct parsedLine{
  char* tag;
  char* data;
};

void freeParsedLine(parsedLine* line){
  free(line->tag);
  free(line->data);
  free(line);
}

//Parses a string to find the length.
//Allocates the parsed line structure.
//TODO rewrite this as it might leak.
int parseLine(const char* data,int length, parsedLine** result){
  int actualen = strlen(data);
  int safelength = actualen< length ? actualen : length;
  char* temp = (char*) malloc(sizeof(char)*safelength);
  memcpy(temp,data,safelength);
  temp[safelength]=0;
  if(temp[safelength-1]=='\n'){
    temp[safelength-1]=0;
  }

  char* comma = strchr(temp,DELIMITER); 

  //Check if we found seperator
  if(comma==NULL){
    return MISSING_DELIMITER;
  }

  int returncode=0;
  char* extra;
  //Check for more commas
  if((extra=strchr(comma+1,DELIMITER))!=NULL){
    returncode = EXTRA_DELIMITER;
    //Set the first next occurence to delimited.
    //This is because any extra items should be ignored
    extra=0;
  }

  *comma=0;
  *result = (parsedLine*) malloc(sizeof(parsedLine)*1);
  parsedLine* r = *result;
  r->tag=temp;
  r->data=comma+1;

  return returncode;
}

int determineTagType(UA_Client* client, UA_NodeId node, int* result){
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
    *result = type->typeIndex;
  }

  UA_ReadRequest_deleteMembers(&rReq);
  UA_ReadResponse_deleteMembers(&rResp);
  return 0;
}

int writeTag(UA_Client* client, UA_NodeId node, int datatype, void* payload){
  UA_WriteRequest wReq;
  UA_WriteRequest_init(&wReq);
  wReq.nodesToWrite = UA_WriteValue_new();
  wReq.nodesToWriteSize = 1;
  wReq.nodesToWrite[0].nodeId = node;
  wReq.nodesToWrite[0].attributeId = UA_ATTRIBUTEID_VALUE;
  wReq.nodesToWrite[0].value.hasValue = true;
  wReq.nodesToWrite[0].value.value.type = &UA_TYPES[datatype];
  wReq.nodesToWrite[0].value.value.storageType = UA_VARIANT_DATA_NODELETE;
  wReq.nodesToWrite[0].value.value.data = payload;

  UA_WriteResponse wResp = UA_Client_Service_write(client, wReq);

  int retval = wResp.responseHeader.serviceResult;
  UA_WriteRequest_deleteMembers(&wReq);
  UA_WriteResponse_deleteMembers(&wResp);

  return retval;
}

int convertInputToType(int typeid, char* data, void**result){
  int retval = 0;
  switch(typeid){
    case UA_TYPES_UINT16:
    case UA_TYPES_INT32:{
                          char* endptr;
                          int valueasint= (int) strtol(data, &endptr,10);
                          if(*endptr=='\0'){
                            *result= &valueasint;
                          }
                          else{
                            retval = NOT_INT;
                          }
                          break;
                        }
    case UA_TYPES_STRING:{
                           UA_String string = UA_String_fromChars(data);
                           *result= &string;
                           break;
                         }

  }
  return retval;
}

int main(int argc, char** argv) {
  const char* default_url = "opc.tcp://127.0.0.1:49380";
  const char* url = getenv("OPCUA_SERVER");
  url = url ? url: default_url;

  UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout);
  UA_StatusCode retval = UA_Client_connect(client, UA_ClientConnectionTCP,url);

  //TODO: reconnect?
  if(retval!=UA_STATUSCODE_GOOD){
    fprintf(stderr,"Failed to connect");
    return retval;
  }

  char* buffer = (char*) malloc( sizeof(char)*READSIZE);

  //Read stdin
  while(true){

    //Read newline delimited string
    if(fgets(buffer,READSIZE,stdin)==NULL){
      if(feof(stdin)){return 0;}
      fprintf(stderr,"ERROR: %s\n",strerror(errno));
      return -1;
    }

    //TODO: shift to strtok
    parsedLine* parsed;
    if(parseLine(buffer,READSIZE,&parsed)<0){
      fprintf(stderr,"WARN: Skipping due to Unable to understand line format: %s", buffer); 
      freeParsedLine(parsed);
      continue;
    }

    //Node to read
    UA_NodeId node=UA_NODEID_STRING_ALLOC(2,parsed->tag);

    int typeid;
    if(determineTagType(client,node,&typeid)){
      fprintf(stderr,"Unable to determine tag type: %s\n",parsed->tag);
    }
    else{
      void* data=NULL;
      if(convertInputToType(typeid,parsed->data,&data)){
        fprintf(stderr,"Error casting type for %s. OPCTypeId:%d, Check OPC Server type matches\n",parsed->data,typeid);
      }
      if(data!=NULL && writeTag(client,node,typeid,data)){
        fprintf(stderr,"Failed to write tag %s.",parsed->data);
      }

    }
    freeParsedLine(parsed);
  }
  UA_Client_disconnect(client);
  UA_Client_delete(client);
}

