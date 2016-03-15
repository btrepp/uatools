#include <stdio.h>

#include "open62541.h"

int main(int argc, char** argv) {
	UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout);

	//listing endpoints
	UA_EndpointDescription* endpointArray = NULL;
	size_t endpointArraySize = 0;
	UA_StatusCode retval =
		UA_Client_getEndpoints(client, UA_ClientConnectionTCP, argv[1],
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
