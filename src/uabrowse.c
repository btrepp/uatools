#include <stdio.h>

#include "open62541.h"

int main(int argc, char** argv) {
	UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout);
	UA_StatusCode retval = UA_Client_connect(client, UA_ClientConnectionTCP,argv[1]);
	if(retval != UA_STATUSCODE_GOOD) {
		UA_Client_delete(client);
		return retval;
	}

	// Browse some objects
	printf("Browsing nodes in objects folder:\n");

	UA_BrowseRequest bReq;
	UA_BrowseRequest_init(&bReq);
	bReq.requestedMaxReferencesPerNode = 0;
	bReq.nodesToBrowse = UA_BrowseDescription_new();
	bReq.nodesToBrowseSize = 1;
	bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); //browse objects folder
	bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; //return everything


	UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
	printf("%-9s %-16s %-16s %-16s\n", "NAMESPACE", "NODEID", "BROWSE NAME", "DISPLAY NAME");
	for (size_t i = 0; i < bResp.resultsSize; ++i) {
		for (size_t j = 0; j < bResp.results[i].referencesSize; ++j) {
			UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
			if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
				printf("%-9d %-16d %-16.*s %-16.*s\n", ref->browseName.namespaceIndex,
						ref->nodeId.nodeId.identifier.numeric, (int)ref->browseName.name.length,
						ref->browseName.name.data, (int)ref->displayName.text.length,
						ref->displayName.text.data);
			} else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) {
				printf("%-9d %-16.*s %-16.*s %-16.*s\n", ref->browseName.namespaceIndex,
						(int)ref->nodeId.nodeId.identifier.string.length, ref->nodeId.nodeId.identifier.string.data,
						(int)ref->browseName.name.length, ref->browseName.name.data,
						(int)ref->displayName.text.length, ref->displayName.text.data);
			}
			//TODO: distinguish further types
		}
	}
	UA_BrowseRequest_deleteMembers(&bReq);
	UA_BrowseResponse_deleteMembers(&bResp);
	UA_Client_disconnect(client);
	UA_Client_delete(client);
	return 0;
}
