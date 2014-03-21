#include "file_loader.h"
#include <assert.h>
#include "zlog.h"
#include "thread_pool.h"

	zlog_category_t *category;
	char *files[3];

void *read (void *args) {

	file_loader_t *loader = (file_loader_t *)args;
	char buffer[128];

	int fd = loader->open (loader, files[1]);
	loader->read (loader, fd, 12, 21, buffer);
	zlog_debug (category, "READ: %s", buffer);
}

int main () {
	file_loader_t loader;
	thread_pool_t pool;

	int state = zlog_init("../zlog.conf");
	assert (state == 0);
    category = zlog_get_category("default");
	assert (category != NULL);
    zlog_debug (category, "Start!");



	files[0] = "file_loader.h";
	files[1] = "file_loader.c";
	files[2] = "test_file_loader.c";


	init_file_loader (&loader, 3, "..", files);

	init_thread_pool (&pool, 5);



	int i = 0;
	for (; i < 100; i++) {
		pool.submit (&pool, read, &loader);
	}


	sleep (1);
	destroy_thread_pool (&pool);

	destroy_file_loader (&loader);

}