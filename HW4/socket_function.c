#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include "response.h"
#include "request.h"

void send_request_t (int sd, request_t* rq){
	char buf1[10];
	sprintf(buf1,"%d",rq->client_id);
	write(sd,buf1, sizeof(buf1));

	char buf2[10];
	sprintf(buf2,"%d",rq->priority);
	write(sd, buf2, sizeof(buf2));
	
	char buf3[10];
	sprintf(buf3,"%d",rq->client_seq_id);
	write(sd, buf3, sizeof(buf3));
}

void read_request_t(int sd, request_t* rq){	
	char client_id[10];		
	char priority[10];
	char client_seq_id[10];

	read (sd, &client_id, 10);
	rq->client_id = atoi(client_id);
	
	read (sd, &priority, 10);
	rq->priority = atoi(priority);

	read (sd, &client_seq_id, 10); 
	rq->client_seq_id = atoi(client_seq_id);
}
	
void send_response_t(int sd, response_t* rp){
	char buf1[10];
	sprintf(buf1,"%d",rp->worker_id);
	write(sd,buf1, sizeof(buf1));
}

void read_response_t(int sd, response_t* rp){
	
	char worker_id[10];
	read (sd, &worker_id, 10);
	rp->worker_id = atoi(worker_id);
}


	

