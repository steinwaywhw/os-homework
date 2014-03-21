#include "response.h"
#include "request.h"

void send_request_t (int sd, request_t* rq);
void read_request_t(int sd, request_t* rq);
void read_response_t(int sd, response_t* rp);
void send_response_t(int sd, response_t* rp);
