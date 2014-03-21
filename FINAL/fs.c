#define _KERNEL_MODE

#ifndef _KERNEL_MODE
	// #include <assert.h>
	#include <unistd.h>
	#include <malloc.h>
	#include <string.h>
	#include <stdio.h>
#else
	#include <linux/string.h>
	#include <linux/vmalloc.h>
	#include <linux/module.h>         /* Needed for the macros */
	MODULE_LICENSE("GPL");

#endif

#include "config.h"
#include "fs.h"



fs_t* init_fs () {
	fs_t *fs = (fs_t *)malloc(sizeof(fs_t));
	// assert (fs != NULL);

	memset (fs, 0, sizeof(fs_t));

	device_t *device = &fs->device;

	// allocate device
	device->start = malloc (DISK_SIZE_IN_KB * 1024);
	// assert (device->start != NULL);
	device->limit = device->start + DISK_SIZE_IN_KB * 1024;
	memset (device->start, 0, DISK_SIZE_IN_KB * 1024);

	// setup device
	device->locate = device_locate;

	// allocate super block;
	fs->super_block = device->locate (device, OFFSET_SUPER_BLOCK);
	super_block_t *sb = fs->super_block;
	// assert (sb != NULL);

	// setup super block
	sb->fs = fs;

	sb->inodes = device->locate (device, OFFSET_INODE_ARRAY);
	sb->bitmap = device->locate (device, OFFSET_BLOCK_BITMAP);
	sb->blocks = device->locate (device, OFFSET_FREE_BLOCK);
	
	sb->inode_ops.start = device->locate (device, OFFSET_INODE_ARRAY);
	sb->inode_ops.limit = device->locate (device, OFFSET_BLOCK_BITMAP);
	sb->inode_ops.allocate = NULL;
	sb->inode_ops.free = NULL;

	sb->bitmap_ops.start = device->locate (device, OFFSET_BLOCK_BITMAP);
	sb->bitmap_ops.limit = device->locate (device, OFFSET_FREE_BLOCK);
	sb->bitmap_ops.test = bitmap_test;
	sb->bitmap_ops.set = bitmap_set_;
	sb->bitmap_ops.clear = bitmap_clear_;
	sb->bitmap_ops.clear_all = bitmap_clear_all;
	sb->bitmap_ops.first_free = bitmap_first_free;
	sb->bitmap_ops.clear_all (&sb->bitmap_ops);

	sb->free_inodes = INODE_ARRAY_SIZE;
	sb->free_blocks = OFFSET_LIMIT - OFFSET_FREE_BLOCK;

	sb->lookup = NULL;

	// setup root inode
	index_node_t *root = &sb->inodes[INODE_ROOT_INDEX];
	root->in_use = 1;
	sb->free_inodes--;
	strcpy (root->type, INODE_TYPE_DIR);
	root->size = 0;

	return fs;
}

int destroy_fs (fs_t *fs) {
	// assert (fs != NULL);

	free (fs->device.start);
	free (fs);

	return 0;
}

unsigned int bitmap_first_free (struct bitmap_ops_t *op) {
	int i;
	for (i = 0; i < OFFSET_LIMIT - OFFSET_FREE_BLOCK; i++) {
		if (op->test (op, i) == 0)
			return i;
	}
	return -1;
} 

unsigned int bitmap_test (struct bitmap_ops_t *op, int free_block_number ) {
	// assert (free_block_number >= 0 && free_block_number + OFFSET_FREE_BLOCK < OFFSET_LIMIT);

	char *p = (char *)op->start;
	char data;
	int bits = sizeof (char) * 8;

	p += free_block_number / bits;
	data = *p;

	return (data >> (free_block_number % bits)) & 1;
}

unsigned int bitmap_set_ (struct bitmap_ops_t *op, int free_block_number) {
	// assert (free_block_number >= 0 && free_block_number + OFFSET_FREE_BLOCK < OFFSET_LIMIT);

	char *p = (char *)op->start;
	char data;
	int bits = sizeof (char) * 8;
	p += free_block_number / bits;
	data = *p;

	int old = (data >> (free_block_number % bits)) & 1;	

	*p |= 1 << (free_block_number % bits);

	return old;
}


unsigned int bitmap_clear_ (struct bitmap_ops_t *op, int free_block_number) {
	// assert (free_block_number >= 0 && free_block_number + OFFSET_FREE_BLOCK < OFFSET_LIMIT);

	char *p = (char *)op->start;
	char data;
	int bits = sizeof (char) * 8;
	p += free_block_number / bits;
	data = *p;

	int old = (data >> (free_block_number % bits)) & 1;	

	*p &= ~(1 << (free_block_number % bits));
	
	return old;

} 
unsigned int bitmap_clear_all (struct bitmap_ops_t *op) {
	memset (op->start, 0, op->limit - op->start);
	return 0;
}


int inode_create (super_block_t *sb, char *pathname, char *type) {
	// assert (sb != NULL);
	// assert (pathname != NULL);

	// assert (sb->free_inodes > 0);

	// assume parent exists, and child not exists.
	int parent = inode_lookup_parent (sb, pathname);
	// assert (parent >= 0);
	int child = inode_lookup_full (sb, pathname);
	// assert (child < 0);

	// parent dir dentry
	dir_entry_t dentry;
	memset (&dentry, 0, sizeof (dentry));

	// chlid dir/reg inode
	int inode = inode_allocate (sb);
	// assert (inode > 0);
 	strcpy (sb->inodes[inode].type, type);
 	sb->inodes[inode].in_use = 1;

 	// get dirname
 	char buffer[MAX_FILE_SIZE];
	int count = path_explode (pathname, buffer);
	char *p = path_get_component (buffer, count - 1);

 	dentry.inode = inode;
 	strcpy (dentry.filename, p);

 	// write to parent
 	int status = fs_append (sb->fs, parent, &dentry, sizeof (dentry));

 	return status;
}

int fs_mk (fs_t *fs, char *pathname, char *type) {
	// assert (fs != NULL);
	// assert (pathname != NULL);

	super_block_t *sb = fs->super_block;

	// can't make
	if (sb->free_inodes == 0)
		return -1;
	if (sb->free_blocks == 0)
		return -1;
	if (strlen (pathname) > MAX_FILE_FULL)
		return -1;

	// parent not exists
	int parent = inode_lookup_parent (fs->super_block, pathname);
	if (parent < 0)
		return -1;

	// child already exists
	int child = inode_lookup_full (fs->super_block, pathname);
	if (child >= 0)
		return -1;

	int status = inode_create (fs->super_block, pathname, type);

	if (status < 0)
		return -1;

	return 0;
}

int fs_rm (fs_t *fs, char *pathname) {
	// assert (fs != NULL);
	// assert (pathname != NULL);

	// parent not exists
	int parent = inode_lookup_parent (fs->super_block, pathname);
	if (parent < 0)
		return -1;

	// child not exists
	int child = inode_lookup_full (fs->super_block, pathname);
	if (child < 0)
		return -1;

	// child is non-empty dir
	if (inode_isdir (fs->super_block, child) && !inode_isdir_isempty (fs->super_block, child))
		return -1;

	// child is empty-dir or child is reg file
	return inode_rm (fs->super_block, pathname);
}

int inode_rm (super_block_t *sb, char *pathname) {
	// assert (sb != NULL);
	// assert (pathname != NULL);

	// assume parent exists
	int parent = inode_lookup_parent (sb, pathname);
	// assert (parent >= 0);

	// assume child exists, and it is an empty entry
	int child = inode_lookup_full (sb, pathname);
	// assert (child >= 0);
	// assert (inode_isdir_isempty (sb, child) || inode_isreg (sb, child));

	// get dirname
 	char buffer[MAX_FILE_SIZE];
	int count = path_explode (pathname, buffer);
	char *p = path_get_component (buffer, count - 1);

	// search for parent dir dentry
	int index = inode_lookup_dentry (sb, parent, p);

	// remove it
	inode_remove_dentry (sb, parent, index);

	// chlid dir inode
	inode_free (sb, child);

 	return 0;
}

int inode_isdir_isempty (super_block_t *sb, int index) {
	return sb->inodes[index].size == 0 && inode_isdir (sb, index);
}

int inode_isdir (super_block_t *sb, int index) {
	return strcmp (sb->inodes[index].type, INODE_TYPE_DIR) == 0;
}

int inode_isreg (super_block_t *sb, int index) {
	return strcmp (sb->inodes[index].type, INODE_TYPE_REG) == 0;
}

int inode_lookup_dentry (super_block_t *sb, int inode, char *component) {

	// assert (sb != NULL);
	// assert (inode_isdir (sb, inode));
	// assert (component != NULL);

	dir_entry_t dentry;
	int offset = 0;
	int index = 0;
	int status = fs_read (sb->fs, inode, offset, &dentry, sizeof (dentry));

	while (status == sizeof(dentry)) {
		if (strcmp (dentry.filename, component) == 0)
			break;

		index++;
		offset += sizeof (dentry);
		status = fs_read (sb->fs, inode, offset, &dentry, sizeof (dentry));
	}

	if (status != sizeof (dentry))
		return -1;

	return index;
}

int inode_remove_dentry (super_block_t *sb, int inode, int dentry) {

	// assert (sb != NULL);

	// assume dir
	// assert (inode_isdir (sb, inode));

	// assume exists
	// assert (sb->inodes[inode].size >= dentry * sizeof (dir_entry_t));

	dir_entry_t buffer;
	int offset = (dentry + 1) * sizeof (dir_entry_t);
	
	int status;
	status = fs_read (sb->fs, inode, offset, &buffer, sizeof (dir_entry_t));

	while (status == sizeof (dir_entry_t)) {
		fs_write (sb->fs, inode, offset - sizeof (dir_entry_t), &buffer, sizeof (dir_entry_t));
		offset += sizeof (dir_entry_t);
		status = fs_read (sb->fs, inode, offset, &buffer, sizeof (dir_entry_t));
	}

	inode_shrink (sb, inode, sb->inodes[inode].size - sizeof (dir_entry_t));

	return 0;
}

int inode_lookup (super_block_t *sb, int parent, char *childname) {
	// assert (sb != NULL);
	// assert (childname != NULL);

	index_node_t *inode = &sb->inodes[parent];
	
	// not exist
	if (inode->in_use == 0)
		return -1;

	// not a dir
	if (strcmp (inode->type, INODE_TYPE_DIR) != 0)
		return -1;

	dir_entry_t dentry;
	int offset = 0;
	int status = fs_read (sb->fs, parent, offset, &dentry, sizeof (dentry));
		
	// empty dir	
	if (status == 0)
		return -1;

	while (status == sizeof(dentry)) {
		if (strcmp (dentry.filename, childname) == 0)
			return dentry.inode;

		offset += sizeof (dentry);
		status = fs_read (sb->fs, parent, offset, &dentry, sizeof (dentry));
	}

	// not found
	return -1;
}

int inode_lookup_full (super_block_t *sb, char *fullname) {
	// assert (sb != NULL);
	// assert (fullname != NULL);

	if (strcmp (fullname, "/") == 0)
		return 0;

	int index = 0;

	// assert (strlen (fullname) < MAX_FILE_SIZE);
	char *buffer = malloc (MAX_FILE_SIZE * sizeof(char));
	int count = path_explode (fullname, buffer);

	char *p = buffer;
	int i;
	for (i = 0; i < count; i++) {
		int len = strlen (p);
		index = inode_lookup (sb, index, p);

		// not found
		if (index == -1) {
			free (buffer);
			return -1;
		}

		p += len + 1;
	}

	free (buffer);
	return index;
}

int inode_lookup_parent (super_block_t *sb, char *child) {
	// assert (sb != NULL);
	// assert (child != NULL);

	if (strcmp (child, "/") == 0)
		return 0;

	int index = 0;

	// assert (strlen (child) < MAX_FILE_SIZE);
	char *buffer = malloc (MAX_FILE_SIZE * sizeof(char));
	int count = path_explode (child, buffer);

	char *p = buffer;
	int i;
	for (i = 0; i < count - 1; i++) {
		int len = strlen (p);
		index = inode_lookup (sb, index, p);

		// not found or found but not dir
		if (index == -1 || strcmp (sb->inodes[index].type, INODE_TYPE_DIR) != 0) {
			free (buffer);
			return -1;
		}

		p += len + 1;
	}

	free (buffer);
	return index;
}

int inode_allocate (super_block_t *sb) {
	// assert (sb != NULL);

	if (sb->free_inodes == 0)
		return -1;

	int i;
	for (i = 1; i < INODE_ARRAY_SIZE; i++) {
		if (sb->inodes[i].in_use == 0) {
			memset (&sb->inodes[i], 0, sizeof (index_node_t));
			sb->inodes[i].in_use == 1;
			sb->free_inodes--;
			return i;
		}
	}

	return -1;
}

int inode_free (super_block_t *sb, int index) {
	// assert (sb != NULL);

	memset (&sb->inodes[index], 0, sizeof (index_node_t));
	sb->free_inodes++;

	return 0;
}

char* path_get_component (char *components, int index) {
	char *p = components;
	int len;

	int i;
	for (i = 0; i < index; i++) {
		len = strlen (p);
		p += len + 1;
	}

	return p;
}

int path_explode (char *path, char *components) {
	// assert (path != NULL && components != NULL);

	char *p = path;
	char *q = components;

	// start from root
	// assert (*p == PATH_DELIMITER_CHAR);

	// skip root
	p++;

	// copy, and replace / with \0
	int count = 0;
	while (*p != 0) {
		if (*p == PATH_DELIMITER_CHAR)
			*q = '\0', count++;
		else 
			*q = *p;

		p++;
		q++;
	}

	// revise the count
	if (*(p-1) != PATH_DELIMITER_CHAR)
		count++;

	// append at the end
	*q = '\0';

	return count;
}

int fs_read (fs_t *fs, int index, int offset, void *buffer, int len) {
	// assert (fs != NULL);
	// assert (index >= 0 && index < INODE_ARRAY_SIZE);
	// assert (buffer != NULL);
	// assert (offset >= 0);
	// assert (len >= 0);

	index_node_t *inode = &fs->super_block->inodes[index];
	// assert (inode->in_use == 1);

	// EOF
	if (offset >= inode->size)
		return 0;

	int to_read = inode->size - offset;
	to_read = to_read > len ? len : to_read;

	return inode_read (fs->super_block, index, offset, buffer, to_read);
}

int fs_write (fs_t *fs, int index, int offset, void *buffer, int len) {
	// assert (fs != NULL);
	// assert (index >= 0 && index < INODE_ARRAY_SIZE);
	// assert (buffer != NULL);
	// assert (offset >= 0);
	// assert (len >= 0);

	index_node_t *inode = &fs->super_block->inodes[index];
	// assert (inode->in_use == 1);

	if (offset + len > LOC_LIMIT_DOUBLE_INDIRECT)
		return -1;

	int status;
	if (inode->size < offset + len) {
		status = inode_expand (fs->super_block, index, offset + len);
		if (status < 0)
			return -1;
	}

	return inode_write (fs->super_block, index, offset, buffer, len);
}

int fs_append (fs_t *fs, int index, void *buffer, int len) {
	// assert (fs != NULL);
	// assert (index >= 0 && index < INODE_ARRAY_SIZE);
	// assert (buffer != NULL);
	// assert (len >= 0);

	index_node_t *inode = &fs->super_block->inodes[index];
	// assert (inode->in_use == 1);

	int prev = inode->size;
	int status = fs_write (fs, index, inode->size, buffer, len);
	// assert (inode->size == prev + status);

	return status;
}

int inode_append (super_block_t *sb, int index, void *buffer, int len) {
	// assert (sb != NULL);
	// assert (index >= 0 && index < INODE_ARRAY_SIZE);
	// assert (buffer != NULL);
	// assert (len >= 0);

	int offset = sb->inodes[index].size;
	return inode_write (sb, index, offset, buffer, len);
}

int inode_write (super_block_t *sb, int index, int offset, void *buffer, int len) {
	// assert (sb != NULL);
	// assert (buffer != NULL);
	// assert (len >= 0);
	// assert (index >= 0);
	// assert (index < INODE_ARRAY_SIZE);
	// assert (offset >= 0);

	int total_len = len;

	index_node_t *inode = &sb->inodes[index];
	location_t *location = &inode->location;

	// assert (inode->size >= (offset + len));

	void *src, *dst;
	int to_copy;

	// first 
	src = buffer;
	dst = loc_locate (location, offset);
	to_copy = (offset / BLOCK_SIZE_IN_B + 1) * BLOCK_SIZE_IN_B - offset;
	to_copy = to_copy > len ? len : to_copy;
	// printf ("Writing: %p, %d\n", dst, to_copy);

	memcpy (dst, src, to_copy);

	offset += to_copy;
	src += to_copy;
	len -= to_copy;

	// second to second last
	while (len > BLOCK_SIZE_IN_B) {
		dst = loc_locate (location, offset);
	// printf ("Writing: %p, %d\n", dst, BLOCK_SIZE_IN_B);
		memcpy (dst, src, BLOCK_SIZE_IN_B);

		src += BLOCK_SIZE_IN_B;
		offset += BLOCK_SIZE_IN_B;
		len -= BLOCK_SIZE_IN_B;
	}

	// last
	dst = loc_locate (location, offset);
	// printf ("Writing: %p, %d\n", dst, len);
	memcpy (dst, src, len);

	return total_len;
}

int inode_read (super_block_t *sb, int index, int offset, void *buffer, int len) {
	// assert (sb != NULL 
	// assert (buffer != NULL);
	// assert (len >= 0);
	// assert (index >= 0);
	// assert (index < INODE_ARRAY_SIZE);
	// assert (offset >= 0);

	index_node_t *inode = &sb->inodes[index];
	location_t *location = &inode->location;

	int total_len = len;

	// assert (inode->size >= (offset + len));

	void *src, *dst;
	int to_copy;

	// first 
	dst = buffer;
	src = loc_locate (location, offset);
	to_copy = (offset / BLOCK_SIZE_IN_B + 1) * BLOCK_SIZE_IN_B - offset;
	to_copy = to_copy > len ? len : to_copy;
	memcpy (dst, src, to_copy);

	offset += to_copy;
	dst += to_copy;
	len -= to_copy;

	// second to second last
	while (len > BLOCK_SIZE_IN_B) {
		src = loc_locate (location, offset);
		memcpy (dst, src, BLOCK_SIZE_IN_B);

		dst += BLOCK_SIZE_IN_B;
		offset += BLOCK_SIZE_IN_B;
		len -= BLOCK_SIZE_IN_B;
	}

	// last
	src = loc_locate (location, offset);
	memcpy (dst, src, len);

	return total_len;
}

int inode_resize (super_block_t *sb, int index, int size) {
	// assert (sb != NULL);
	// assert (index >=0 && index < INODE_ARRAY_SIZE);
	// assert (size >= 0 && size < MAX_FILE_SIZE);

	int status = size;
	index_node_t *inode = &sb->inodes[index];

	// allocate the first block
	if (inode->location.direct[0] == NULL && size != 0) {
		inode->location.direct[0] == fs_allocate_block (sb->fs);
	}

	// within the same block
	if ((size - 1) / BLOCK_SIZE_IN_B == (inode->size - 1) / BLOCK_SIZE_IN_B) {
		inode->size = size;
		return size;
	}

	// do real resize
	if (size < sb->inodes[index].size) 
		status = inode_shrink (sb, index, size);
	
	if (size > sb->inodes[index].size)
		status = inode_expand (sb, index, size);

	return status;
}

int inode_shrink (super_block_t	*sb, int index, int size) {
	// assert (sb != NULL);
	// assert (index >=0 && index < INODE_ARRAY_SIZE);
	// assert (size >= 0);

	index_node_t *inode = &sb->inodes[index];
	// assert (size < inode->size);

	// within the same block
	if ((size - 1) / BLOCK_SIZE_IN_B == (inode->size - 1) / BLOCK_SIZE_IN_B) {
		inode->size = size;
		return size;
	}

	location_t *location = &inode->location;
	location_index_t location_index;

	// current_offset offset, init to the next block
	int current_offset = ((size - 1) / BLOCK_SIZE_IN_B + 1) * BLOCK_SIZE_IN_B;
	if (size == 0)
		current_offset = 0;

	while (current_offset < LOC_LIMIT_DOUBLE_INDIRECT) {

		loc_index (location, current_offset, &location_index);

		// not allocated, no need to shrink any more
		if (loc_locate (location, current_offset) == NULL)
			break;

		// 1 level shrink
		if (current_offset < LOC_LIMIT_DIRECT) {
			inode_shrink_1_level (sb, &location->direct[location_index.level_1]);
			current_offset += BLOCK_SIZE_IN_B;
			continue;
		}

		// 2 level shrink
		if (current_offset < LOC_LIMIT_SINGLE_INDIRECT) {
			void **p = (void **)location->single_indirect[location_index.level_1];
			p += location_index.level_2;
			inode_shrink_1_level (sb, p);

			// check to free pointer block after freeing the 64th pointer
			if (location_index.level_2 == LOC_POINTER_PER_BLOCK - 1) {
				p = (void **)location->single_indirect[location_index.level_1];
				if (*p == NULL)
					inode_shrink_1_level (sb, &location->single_indirect[location_index.level_1]);
			}

			current_offset += BLOCK_SIZE_IN_B;
			continue;
		}

		// three level shrink
		if (current_offset < LOC_LIMIT_DOUBLE_INDIRECT) {
			void **p = (void **)location->double_indirect[location_index.level_1];
			p += location_index.level_2;

			void **q = (void **)*p + location_index.level_3;
			inode_shrink_1_level (sb, q);

			// check to free level-2 pointer block
			if (location_index.level_3 == LOC_POINTER_PER_BLOCK - 1) {
				q = (void **)*p;
				if (*q == NULL)
					inode_shrink_1_level (sb, p);
			}

			// check to free level-1 pointer block
			if (location_index.level_2 == LOC_POINTER_PER_BLOCK - 1) {
				p = (void **)location->double_indirect[location_index.level_1];
				if (*p == NULL)
					inode_shrink_1_level (sb, &location->double_indirect[location_index.level_1]);
			}

		}
	}

	inode->size = size;

 	return size;
} 

int inode_shrink_1_level (super_block_t *sb, void **p) {
	void *block = *p;
	fs_free_block (sb->fs, fs_addr_to_free_block_number (sb->fs, block));
	*p = NULL;

	return 0;
}


// expend an inode to a given size
int inode_expand (super_block_t *sb, int index, int size) {
	// assert (sb != NULL);
	// assert (index >=0 && index < INODE_ARRAY_SIZE);
	// assert (size <= MAX_FILE_SIZE);

	// no free blocks;
	if (sb->free_blocks == 0)
		return -1;

	// get inode
	index_node_t *inode = &sb->inodes[index];
	// assert (size > inode->size);

	// allocate the first block
	if (inode->location.direct[0] == NULL && size != 0) {
		inode->location.direct[0] = fs_allocate_block (sb->fs);
	}

	// the offset is within the same block, no need to expand
	if ((size - 1) / BLOCK_SIZE_IN_B == (inode->size - 1) / BLOCK_SIZE_IN_B) {
		inode->size = size;
		return size;
	}

	location_t *location = &inode->location;

	// allocation counter
	int current_offset = 0;

	location_index_t location_index;
	void *p = NULL, *status = NULL;

	// traverse and allocate all
	while (current_offset < size) {

		// get current_offset allocation index
		loc_index (location, current_offset, &location_index);

		// one level allocation
		if (current_offset < LOC_LIMIT_DIRECT) {

			// allocate data block
			if (loc_locate (location, current_offset) == NULL) {
				status = fs_allocate_block (sb->fs);
				// assert (status != NULL);

				location->direct[location_index.level_1] = status;
			}
			
			current_offset += BLOCK_SIZE_IN_B;
			continue;
		}

		// two level allocation
		if (current_offset < LOC_LIMIT_SINGLE_INDIRECT) {

			if (loc_locate (location, current_offset) == NULL) {
				
				// level 1 allocation
				p = location->single_indirect[location_index.level_1];
				if (p == NULL) {
					status = fs_allocate_block (sb->fs);
					// assert (status != NULL);
					location->single_indirect[location_index.level_1] = fs_allocate_block (sb->fs);
				}

				// data block allocation
				p = location->single_indirect[location_index.level_1];
				p = (void *)((void**)p + location_index.level_2);
				if (*(void **)p == NULL)
					*(void **)p = fs_allocate_block (sb->fs);
			}

			current_offset += BLOCK_SIZE_IN_B;
			continue;
		}

		// three level allocation
		if (current_offset < LOC_LIMIT_DOUBLE_INDIRECT) {

			if (loc_locate (location, current_offset) == NULL) {
				
				// level 1 allocation
				p = location->double_indirect[location_index.level_1];
				if (p == NULL) {
					p = fs_allocate_block (sb->fs);
					// assert (p != NULL);
					location->double_indirect[location_index.level_1] = p;
				}

				// level 2 allocation
				p = (void *)((void **)p + location_index.level_2);
				if (*(void **)p == NULL) {
					*(void **)p = fs_allocate_block (sb->fs);
				}

				// data block allocation
				p = *(void **)p;
				p = (void *)((void **)p + location_index.level_3);
				if (*(void **)p == NULL)
					*(void **)p = fs_allocate_block (sb->fs);
			}

			current_offset += BLOCK_SIZE_IN_B;
			continue;
		}  

		// assert (0);
	}

	inode->size = size;

	return size;
}

// allocate a block, init with all zero
void *fs_allocate_block (fs_t *fs) {
	// assert (fs != NULL);

	// can't allocate
	if (fs->super_block->free_blocks == 0)
		return NULL;

	// compute and get a free block
	bitmap_ops_t *bitmap_ops = &fs->super_block->bitmap_ops;
	int block = bitmap_ops->first_free (bitmap_ops);

	// set bitmap
	bitmap_ops->set (bitmap_ops, block);
	// update counter
	fs->super_block->free_blocks--;

	// init with zero
	memset (fs->device.locate (&fs->device, OFFSET_FREE_BLOCK + block), 0, BLOCK_SIZE_IN_B);

	return fs->device.locate (&fs->device, OFFSET_FREE_BLOCK + block);
}

// free an allocated block, erase its content
int fs_free_block (fs_t *fs, int free_block_number) {
	// assert (fs != NULL);
	// assert (free_block_number >= 0 && free_block_number < OFFSET_LIMIT - OFFSET_FREE_BLOCK);

	// clear bitmap
	bitmap_ops_t *bitmap_ops = &fs->super_block->bitmap_ops;
	bitmap_ops->clear (bitmap_ops, free_block_number);

	// erase
	memset (fs->device.locate (&fs->device, OFFSET_FREE_BLOCK + free_block_number), 0, BLOCK_SIZE_IN_B);

	// update counter
	fs->super_block->free_blocks++;

	return 0;
}

int fs_addr_to_free_block_number (fs_t *fs, void *free_block_addr) {
	return (free_block_addr - fs->super_block->blocks) / BLOCK_SIZE_IN_B;
}

void* fs_abs_block_number_to_addr (fs_t *fs, int abs_block_number) {
	return fs->device.locate (&fs->device, abs_block_number);
}

void* device_locate (device_t *device, int absolute_block_number) {
	return (void *)((char *)device->start + absolute_block_number * BLOCK_SIZE_IN_B);
}


// locate the offset according to location object
void* loc_locate (location_t *location, int offset) {

	// assert (location != NULL);
	// assert (offset >= 0 && offset < LOC_LIMIT_DOUBLE_INDIRECT);

	location_index_t index;

	// get the index first
	loc_index (location, offset, &index);
	void *p = NULL;

	// one level indexing
	if (offset < LOC_LIMIT_DIRECT) {

		p = location->direct[index.level_1];

		// not allocated yet
		if (p == NULL)
			return NULL;

		// allocated, offset by byte
		return (void *)((char *)p + (offset % BLOCK_SIZE_IN_B));
	}
		
	// two level indexing
	if (offset < LOC_LIMIT_SINGLE_INDIRECT) {

		// not allocated yet
		p = location->single_indirect[index.level_1];
		if (p == NULL)
			return NULL;

		// still not allocated yet, offset by 4 bytes
		p = (void *)((void **)p + index.level_2);
		p = *(void **)p;
		if (p == NULL)
			return NULL;

		// allocated, offset by byte
		return (void *)((char *)p + (offset % BLOCK_SIZE_IN_B));
	}

	// three level indexing

	// not allocated yet
	p = location->double_indirect[index.level_1];
	if (p == NULL)
		return NULL;

	// still not allocated yet
	p = (void *)((void **)p + index.level_2);
	p = *(void **)p;
	if (p == NULL)
		return NULL;

	// still not allocated yet
	p = (void *)((void **)p + index.level_3);
	p = *(void **)p;
	if (p == NULL)
		return NULL;

	// allocated, offset by byte
	return (void *)((char *)p + (offset % BLOCK_SIZE_IN_B));
}


// compute the expected index for indexing location object of a given offset
int loc_index (location_t *location, int offset, location_index_t *index) {
	
	// assert (location != NULL);
	// assert (offset >= 0 && offset < LOC_LIMIT_DOUBLE_INDIRECT);
	// assert (index != NULL);

	index->level_1 = -1;
	index->level_2 = -1;
	index->level_3 = -1;

	// first level
	if (offset < LOC_LIMIT_DIRECT) {
		index->level_1 = offset / LOC_SIZE_1_LEVEL;
		return 0;
	}

	// second level
	if (offset < LOC_LIMIT_SINGLE_INDIRECT) {
		offset -= LOC_LIMIT_DIRECT;
		index->level_1 = offset / LOC_SIZE_2_LEVEL;

		offset %= LOC_SIZE_2_LEVEL;
		index->level_2 = offset / LOC_SIZE_1_LEVEL;

		return 0;
	}

	// third level
	offset -= LOC_LIMIT_SINGLE_INDIRECT;
	index->level_1 = offset / LOC_SIZE_3_LEVEL;

	offset %= LOC_SIZE_3_LEVEL;
	index->level_2 = offset / LOC_SIZE_2_LEVEL;

	offset %= LOC_SIZE_2_LEVEL;
	index->level_3 = offset / LOC_SIZE_1_LEVEL;

	return 0;
}