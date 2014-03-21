#include "alt_server.h"
#include "socket_manager.h"
#include "zlog.h"
#include <stdio.h>

zlog_category_t *category;
server_t server;

int main () {
	int state = zlog_init("../zlog.conf");
    category = zlog_get_category("default");
    zlog_info (category, "Server starts!");

    init_server (&server);
    server.run (&server);

    while (1) {
        char c = getchar ();
        if (c == 'q')
            break;
    }

    destroy_server (&server);

    zlog_fini ();
}