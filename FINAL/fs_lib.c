#include "fs_lib.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/ioctl.h>

pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
int g_fd = 0;

int rd_creat (char *pathname) {
	int status;
	command_t command = {
		.pathname = pathname,
		.len = strlen (pathname),
		.status = &status
	};
	pthread_mutex_lock (&g_mutex);
	ioctl (g_fd, IOC_CREAT, &command);
	pthread_mutex_unlock (&g_mutex);
	return status;
}

int rd_mkdir (char *pathname) {
	int status;
	command_t command = {
		.pathname = pathname,
		.len = strlen (pathname),
		.status = &status
	};
	pthread_mutex_lock (&g_mutex);
	ioctl (g_fd, IOC_MKDIR, &command);
	pthread_mutex_unlock (&g_mutex);
	return status;

}
int rd_open (char *pathname) {
	int status;
	command_t command = {
		.pathname = pathname,
		.len = strlen (pathname),
		.status = &status
	};
	pthread_mutex_lock (&g_mutex);
	ioctl (g_fd, IOC_OPEN, &command);
	pthread_mutex_unlock (&g_mutex);
	return status;

}
int rd_close (int fd) {
	int status;
	command_t command = {
		.fd = fd,
		.status = &status
	};
	pthread_mutex_lock (&g_mutex);
	ioctl (g_fd, IOC_CLOSE, &command);
	pthread_mutex_unlock (&g_mutex);
	return status;

}
int rd_read (int fd, char *buffer, int len) {
	int status;
	command_t command = {
		.buffer = buffer,
		.len = len,
		.fd = fd,
		.status = &status
	};
	ioctl (g_fd, IOC_READ, &command);
	return status;

}
int rd_write (int fd, char *buffer, int len) {
	int status;
	command_t command = {
		.buffer = buffer,
		.len = len,
		.fd = fd,
		.status = &status
	};
	pthread_mutex_lock (&g_mutex);
	ioctl (g_fd, IOC_WRITE, &command);
	pthread_mutex_unlock (&g_mutex);
	return status;

}
int rd_lseek (int fd, int offset) {
	int status;
	command_t command = {
		.fd = fd,
		.len = offset,
		.status = &status
	};
	ioctl (g_fd, IOC_LSEEK, &command);
	return status;

}
int rd_unlink (char *pathname) {
	int status;
	command_t command = {
		.pathname = pathname,
		.len = strlen (pathname),
		.status = &status
	};
	pthread_mutex_lock (&g_mutex);
	ioctl (g_fd, IOC_UNLINK, &command);
	pthread_mutex_unlock (&g_mutex);
	return status;

}
int rd_readdir (int fd, char *buffer) {
	int status;
	command_t command = {
		.fd = fd,
		.buffer = buffer,
		.status = &status
	};
	ioctl (g_fd, IOC_READDIR, &command);
	return status;

}