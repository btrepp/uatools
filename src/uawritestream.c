#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "open62541.h"

#define MAX_ATTEMPTS 3
#define READSIZE 1024
#define DELIMITER ','

typedef struct node node;
struct node{
  char* tag;
  int ns;
};

void freeNode(node* node){
  free(node->tag);
  free(node);
}

int convertInputToType(const UA_DataType* datatype,const char* data, UA_Variant *result){
  int retval = 0;
  int typeid = datatype->typeIndex;
  switch(typeid){
    case UA_TYPES_UINT16:
    case UA_TYPES_INT32:{
                          char* endptr;
                          int valueasint= (int) strtol(data, &endptr,10);
                          if(*endptr=='\0'){
                            UA_Variant_setScalarCopy(result,&valueasint,&UA_TYPES[typeid]);
                          }
                          else{
                            retval = UA_STATUSCODE_BADTYPEMISMATCH;
                          }
                          break;
                        }
    case UA_TYPES_STRING:{
                           UA_String s= UA_STRING_ALLOC(data);
                           UA_Variant_setScalarCopy(result,&s,&UA_TYPES[UA_TYPES_STRING]);
                           break;
                         }
    case UA_TYPES_FLOAT:{
                          char *endptr;
                          float valueasfloat = (float) strtof(data,&endptr);
                          if(*endptr=='\0'){
                            UA_Variant_setScalarCopy(result,&valueasfloat,&UA_TYPES[UA_TYPES_FLOAT]);
                          }
                          else{
                            retval = UA_STATUSCODE_BADTYPEMISMATCH;
                          }
                          break;
                        }
    default:{
              retval = UA_STATUSCODE_BADTYPEMISMATCH;
          }

  }
  return retval;
}


int readTagType(UA_Client* client, const node* node, const UA_DataType** outdatatype){
  UA_StatusCode ret;
  UA_NodeId uanode = UA_NODEID_STRING(node->ns,node->tag);
  UA_Variant result;
  ret = UA_Client_readValueAttribute(client,uanode,&result);
  *outdatatype = result.type;
  UA_Variant_deleteMembers(&result);

  return ret;
}

int writeTag(UA_Client* client, const node* node, const UA_DataType* datatype, const char* textdata){
  UA_StatusCode ret;
  UA_Variant wiredata;
  if(!(ret=convertInputToType(datatype,textdata,&wiredata))){
    ret=UA_Client_writeValueAttribute(client, UA_NODEID_STRING(node->ns,node->tag),&wiredata);
  }
  UA_Variant_deleteMembers(&wiredata);
  return ret;
}

int main(int argc, char** argv) {
  const char* default_url = "opc.tcp://127.0.0.1:49380";
  const char* url = getenv("OPCUA_SERVER");
  url = url ? url: default_url;
  
  const bool detail = getenv("UAWRITE_DEBUG") ? true: false;

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

    if(detail){fprintf(stderr,"DEBUG: %s", buffer);}

    int ns;
    char* tagstringbuffer= (char*) malloc(strlen(buffer)*sizeof(char));
    char* databuffer = (char*) malloc(strlen(buffer)*sizeof(char));
    int conversions=sscanf(buffer,"ns=%d;s=%[^,],%[^\n]",&ns,tagstringbuffer,databuffer);
    if(conversions!=3 ){
      fprintf(stderr,"WARN: Skipping due to Unable to understand line format: %s", buffer); 
      free(tagstringbuffer);
      free(databuffer);
      continue;
    }

    node tag;
    tag.ns=ns;
    tag.tag=tagstringbuffer;


    int attempt=0;
    do{
      retval=UA_Client_connect(client, UA_ClientConnectionTCP,url);
      const UA_DataType* type;
      if(retval==UA_STATUSCODE_GOOD){
        retval=readTagType(client,&tag,&type);
        if(retval!=UA_STATUSCODE_GOOD){
          fprintf(stderr,"Unable to determine tag type: %s",buffer);
        }
      }
      if(retval==UA_STATUSCODE_GOOD){
        retval=writeTag(client,&tag,type,databuffer);
        if(retval!=UA_STATUSCODE_GOOD){
          fprintf(stderr,"Failed to write tag: %s",buffer);
        }
      }
      attempt++;
    } while(retval!=UA_STATUSCODE_GOOD && attempt<MAX_ATTEMPTS);
    if(attempt>=MAX_ATTEMPTS){
      fprintf(stderr,"Error with line attempted:%d %s",attempt,buffer);
    }

    free(tagstringbuffer);
    free(databuffer);
  }
  UA_Client_disconnect(client);
  UA_Client_delete(client);
}

