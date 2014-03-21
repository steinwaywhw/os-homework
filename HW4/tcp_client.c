#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include "socket_function.h"



int client_init (char *host, int port) {
 
  struct sockaddr_in name;
  struct hostent *hent;
  int sd;
 
  sd = socket (AF_INET, SOCK_STREAM, 0);
	assert (sd>=0);
 
  hent = gethostbyname (host);
  assert (hent!=NULL);
  
  bcopy (hent->h_addr, &name.sin_addr, hent->h_length);
 
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
 
  /* connect port */
	int status;
  status = connect (sd, (struct sockaddr *)&name, sizeof(name));
  assert (status>=0);
 
  return (sd);
}





int main () {

	
	request_t request;
	response_t response;
	
	request.client_id = 1;
  request.priority = 1;
  request.client_seq_id = 1;


  int sd = client_init ("localhost", 5050);

  
  send_request_t(sd, &request);
	read_response_t(sd,&response);
	printf("The worker id is %d\n",response.worker_id);
  
  return 0;
}

