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

typedef struct node node;
struct node{
  char* tag;
  int ns;
};

void freeNode(node* node){
  free(node->tag);
  free(node);
}

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
                            retval = NOT_INT;
                          }
                          break;
                        }
    case UA_TYPES_STRING:{
                           UA_String s= UA_STRING_ALLOC(data);
                           UA_Variant_setScalarCopy(result,&s,&UA_TYPES[UA_TYPES_STRING]);
                           break;
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

    //TODO: shift to strtok
    parsedLine* parsed;
    if(parseLine(buffer,READSIZE,&parsed)<0){
      fprintf(stderr,"WARN: Skipping due to Unable to understand line format: %s", buffer); 
      freeParsedLine(parsed);
      continue;
    }

    node tag;
    tag.ns=2;
    tag.tag=parsed->tag;

    const UA_DataType* type;
    if(readTagType(client,&tag,&type)){
      fprintf(stderr,"Unable to determine tag type: %s\n",parsed->tag);
    }
    else{
      if(writeTag(client,&tag,type,parsed->data)){
        fprintf(stderr,"Failed to write tag %s %s.\n",parsed->tag,parsed->data);
      }

    }
    freeParsedLine(parsed);
  }
  UA_Client_disconnect(client);
  UA_Client_delete(client);
}

