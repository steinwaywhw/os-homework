#define _COMMAND_LINE


#include "server.h"
#include "client.h"
#include "socket_manager.h"
#include "zlog.h"

zlog_category_t *category;
server_t server;
client_t client[10];


int main () {
	int state = zlog_init("../zlog.conf");
	assert (state == 0);
    category = zlog_get_category("default");
	assert (category != NULL);
    zlog_debug (category, "Start!");

    init_server (&server);
    server.run (&server);

    int i, j;
    for (i = 0; i < 10; i++) {
		init_client (&client[i], i+1);
    	client[i].run (&client[i]);
    }

    for (i = 0; i < 10; i++) {
        client[i].start (&client[i], "sw", true);
    }

    client[1].stop (&client[1], "sw");
    client[2].seek (&client[2], "sw", 50);
    client[3].start (&client[3], "no", true);
    sleep (5);

    for (i = 0; i < 10; i++) {
    	destroy_client (&client[i]);
    }

    destroy_server (&server);

    zlog_fini ();
}