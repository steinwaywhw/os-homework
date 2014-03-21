#ifndef _FILELOADER_H
#define _FILELOADER_H

#include "bool.h"

#define MAXFILES 128
#define MAXLENGTH 128

typedef struct file_loader_t {
	char prefix[MAXLENGTH];
	char filenames[MAXFILES][MAXLENGTH];
	char fullfilenames[MAXFILES][MAXLENGTH];
	int *descriptors;
	int count;

	boolean (*exists) (struct file_loader_t *loader, char *filename);
	int (*open) (struct file_loader_t *loader, char *filename);
	int (*read) (struct file_loader_t *loader, int fd, int offset, int count, void *buffer);
} file_loader_t;

int init_file_loader (file_loader_t *loader, int count, char *prefix, char **filenames);
int destroy_file_loader (file_loader_t *loader);

#define ENOTFOUND -1
//#define EOF 0

#endif