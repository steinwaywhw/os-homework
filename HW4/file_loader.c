#include "file_loader.h"
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include "zlog.h"
#include "bool.h"
#include <string.h>
#include <malloc.h>


int _open (struct file_loader_t *loader, char *filename);
int _read (struct file_loader_t *loader, int fd, int offset, int count, void *buffer);
boolean _exists (struct file_loader_t *loader, char *filename);


boolean _exists (struct file_loader_t *loader, char *filename) {
	assert (filename != NULL);
	assert (loader != NULL);

	int i;
	zlog_category_t *category = zlog_get_category ("default");
	//zlog_info (category, "filename : %s", filename);

	for (i = 0; i < loader->count; i++) {
		if (strcmp (filename, loader->filenames[i]) == 0)
			return true;
	}
	//zlog_info (category, "filename : %s, not exists.", filename);
	return false;
}

int _open (struct file_loader_t *loader, char *filename) {
	assert (filename != NULL);
	assert (loader != NULL);

	int i;
	zlog_category_t *category = zlog_get_category ("default");
	//zlog_info (category, "filename : %s", filename);
	for (i = 0; i < loader->count; i++) {
		if (strcmp (filename, loader->filenames[i]) == 0)
			return loader->descriptors[i];
	}

	return ENOTFOUND;
}

int _read (struct file_loader_t *loader, int fd, int offset, int count, void *buffer) {
	assert (loader != NULL);
	assert (fd > 0);
	assert (offset >= 0);
	assert (count >= 0);
	assert (buffer != NULL);

	zlog_category_t *category;
	category = zlog_get_category ("file_loader");
	assert (category != NULL);

	int status;
	status = pread (fd, buffer, count, offset);

	if (status == 0)
		return EOF;

	assert (status <= count);
	return status;
}


int init_file_loader (file_loader_t *loader, int count, char *prefix, char **filenames) {
	assert (filenames != NULL);
	assert (loader != NULL);
	assert (count > 0);
	assert (count <= MAXFILES);

	if (prefix == NULL)
		prefix = "";

	zlog_category_t *category;
	category = zlog_get_category ("file_loader");
	assert (category != NULL);

	zlog_info (category, "Initializing file loader with %d files.", count);

	int i, fd;
	int status;
	int err;

	loader->count = count;

	// copy prefix and add the last '/'
	strcpy (loader->prefix, prefix);
	char ending = prefix[strlen(prefix) - 1];
	if (ending != '/') {
		loader->prefix [strlen (prefix)] = '/';
		loader->prefix [strlen (prefix) + 1] = '\0';
	}
	zlog_info (category, "Prefix: %s", loader->prefix);

 	loader->descriptors = (int *)malloc (count * sizeof (int));
 	assert (loader->descriptors != NULL);

	for (i = 0; i < count; i++) {
		assert (strlen (filenames[i]) < MAXLENGTH);
		assert (filenames[i][0] != '/');

		strcpy (loader->fullfilenames[i], loader->prefix);
		strcat (loader->fullfilenames[i], filenames[i]);

		zlog_debug (category, "Opening %s.", loader->fullfilenames[i]);
		fd = open (loader->fullfilenames[i], O_RDONLY);
		assert (fd != -1);

		strcpy (loader->filenames[i], filenames[i]);
		loader->descriptors[i] = fd;
	}

	loader->exists = _exists;
	loader->open = _open;
	loader->read = _read;

	zlog_info (category, "Initialization finished.");

	return 0;
}

int destroy_file_loader (file_loader_t *loader) {
	assert (loader != NULL);

	zlog_category_t *category;
	category = zlog_get_category ("file_loader");
	assert (category != NULL);

	zlog_info (category, "Destroying file loader.");

	int i;
	int status;
	for (i = 0; i < loader->count; i++) {
		status = close (loader->descriptors[i]);
		assert (status == 0);
	}

	free (loader->descriptors);

	zlog_info (category, "Destruction finished.");
	return 0;
}