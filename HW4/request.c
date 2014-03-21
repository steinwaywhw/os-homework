#include "request.h"
#include <stdio.h>
#include <assert.h>
#include "bool.h"
#include <string.h>

int inline encode_req (request_t *request, char *buffer) { 	

	if (strcmp (request->request, REQ_START) == 0)		
		sprintf (buffer, "%u:%d:%s:%s,%d,%s\0", request->client_id, request->priority, request->request, request->repeat == true ? "true" : "false", request->frame_number, request->movie);

	else if (strcmp (request->request, REQ_STOP) == 0)
		sprintf (buffer, "%u:%d:%s:%s\0", request->client_id, request->priority, request->request, request->movie);

	else if (strcmp (request->request, REQ_SEEK) == 0)
		sprintf (buffer, "%u:%d:%s:%d,%s\0", request->client_id, request->priority, request->request, request->frame_number, request->movie);
	else 
		sprintf (buffer, "INVALID REQUEST!\0");

}																	

int inline decode_req (request_t *request, char *data, int len) {		
	assert (false);													
}							
