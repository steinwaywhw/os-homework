#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <assert.h>
#include "request.h"
#include "response.h"
#include "socket_function.h"

int sd, new_sd;
struct sockaddr_in name, cli_name;
int sock_opt_val = 1;
int cli_len;

int server_init (int port) {

  sd = socket (AF_INET, SOCK_STREAM, 0);
	assert(sd>=0);
	
	int stat1;
  stat1 = setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, (char *) &sock_opt_val,
		  sizeof(sock_opt_val));
	assert (stat1 >= 0); 

  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  
	int stat2;                  
  stat2 = bind (sd, (struct sockaddr *)&name, sizeof(name));
	assert (stat2 >= 0); 

  listen (sd, 5);

	return sd;

}



int main () {

	
  request_t request;
	response_t response;

  int sd = server_init (5050);		/* Server port. */

	for (;;) {
      cli_len = sizeof (cli_name);
      new_sd = accept (sd, (struct sockaddr *) &cli_name, &cli_len);
			assert (new_sd>=0);

      printf ("Assigning new socket descriptor:  %d\n", new_sd);

      if (fork () == 0) {	/* Child process. */
				close (sd);
				
				response.worker_id = 1;

				read_request_t(new_sd,&request);
				send_response_t(new_sd,&response);

				printf ("client_id = %d\n",request.client_id);
				printf ("priority = %d\n", request.priority);
				printf ("client_seq_id = %d\n", request.client_seq_id);

				exit (0);
      }
  }



  return 0;
}
