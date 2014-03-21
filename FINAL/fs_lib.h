#ifndef _FS_LIB_H
#define _FS_LIB_H

#include <pthread.h>

extern pthread_mutex_t g_mutex;
typedef struct command_t {
	int 	fd;
	char 	*pathname;
	char 	*buffer;
	int 	len;
	int 	*status;
} command_t;

#define MAGIC 'k'

#define IOC_READ 	_IOWR (MAGIC, 0, command_t)
#define IOC_WRITE 	_IOWR (MAGIC, 1, command_t)
#define IOC_CREAT 	_IOWR (MAGIC, 2, command_t)
#define IOC_MKDIR	_IOWR (MAGIC, 3, command_t)
#define IOC_OPEN 	_IOWR (MAGIC, 4, command_t)
#define IOC_CLOSE 	_IOWR (MAGIC, 5, command_t)
#define IOC_UNLINK	_IOWR (MAGIC, 6, command_t)
#define IOC_READDIR _IOWR (MAGIC, 7, command_t)
#define IOC_LSEEK	_IOWR (MAGIC, 8, command_t)

int rd_creat (char *pathname);
int rd_mkdir (char *pathname);
int rd_open (char *pathname);
int rd_close (int fd);
int rd_read (int fd, char *buffer, int len);
int rd_write (int fd, char *buffer, int len);
int rd_lseek (int fd, int offset);
int rd_unlink (char *pathname);
int rd_readdir (int fd, char *buffer);


extern int g_fd;

#endif