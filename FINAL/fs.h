#ifndef _FS_H
#define _FS_H
#include "config.h"

//#define bitmap_set 		____bitmap_set
//#define bitmap_test 	____bitmap_test
//#define bitmap_clear 	____bitmap_clear

struct fs_t;

typedef struct location_t {
	void *direct[LOC_NR_OF_DIRECT];
	void *single_indirect[LOC_NR_OF_SINGLE_INDIRECT];
	void *double_indirect[LOC_NR_OF_DOUBLE_INDIRECT];
} location_t;

typedef struct location_index_t {
	int level_1;
	int level_2;
	int level_3;
} location_index_t;

typedef union index_node_t {
	char _fill[INODE_SIZE_IN_B];
	struct {
		unsigned char in_use;
		unsigned char type[INODE_TYPE_SIZE];
		unsigned long size;
		location_t location;
	};
} index_node_t;

typedef struct index_node_ops_t {
	index_node_t *start, *limit;
	int (*allocate) ();
	int (*free) (struct index_node_ops_t *op, int index);
} index_node_ops_t;

typedef struct device_t {
	void *start, *limit;
	void* (*locate) (struct device_t *device, int absolute_block_number);
} device_t;

typedef struct bitmap_ops_t {
	void *start, *limit;
	unsigned int (*first_free) (struct bitmap_ops_t *op);
	unsigned int (*test) (struct bitmap_ops_t *op, int free_block_number);
	unsigned int (*set) (struct bitmap_ops_t *op, int free_block_number);
	unsigned int (*clear) (struct bitmap_ops_t *op, int free_block_number);
	unsigned int (*clear_all) (struct bitmap_ops_t *op); 
} bitmap_ops_t;

typedef struct dir_entry_t {
	unsigned char 	filename[MAX_FILE_COMPONENT];
	unsigned short 	inode;
} dir_entry_t;

typedef struct super_block_t {
	struct fs_t		*fs;

	union index_node_t 	*inodes;
	void 				*bitmap;
	void 				*blocks;

	struct index_node_ops_t	inode_ops;
	struct bitmap_ops_t		bitmap_ops;

	unsigned int 	free_inodes;
	unsigned int 	free_blocks;

	unsigned int (*lookup) (struct super_block_t *sb, unsigned char *fullname);
} super_block_t;

typedef struct fs_t {
	struct device_t 		device;
	struct super_block_t	*super_block;
} fs_t;


unsigned int bitmap_first_free (struct bitmap_ops_t *op); 
unsigned int bitmap_test (struct bitmap_ops_t *op, int free_block_number );
unsigned int bitmap_set_ (struct bitmap_ops_t *op, int free_block_number);
unsigned int bitmap_clear_ (struct bitmap_ops_t *op, int free_block_number); 
unsigned int bitmap_clear_all (struct bitmap_ops_t *op);


int fs_addr_to_free_block_number (fs_t *fs, void *free_block_addr);
void* fs_abs_block_number_to_addr (fs_t *fs, int abs_block_number);
void* device_locate (device_t *device, int absolute_block_number);

char* path_get_component (char *components, int index);
void* loc_locate (location_t *location, int offset);
void *fs_allocate_block (fs_t *fs);



fs_t* init_fs ();

int destroy_fs (fs_t *fs);

int inode_create (super_block_t *sb, char *pathname, char *type);

int fs_mk (fs_t *fs, char *pathname, char *type);

int fs_rm (fs_t *fs, char *pathname);
int inode_rm (super_block_t *sb, char *pathname);

int inode_isdir_isempty (super_block_t *sb, int index);
int inode_isdir (super_block_t *sb, int index);
int inode_isreg (super_block_t *sb, int index);
int inode_lookup_dentry (super_block_t *sb, int inode, char *component);
int inode_remove_dentry (super_block_t *sb, int inode, int dentry);

int inode_lookup (super_block_t *sb, int parent, char *childname) ;
int inode_lookup_full (super_block_t *sb, char *fullname) ;

int inode_lookup_parent (super_block_t *sb, char *child) ;

int inode_allocate (super_block_t *sb);

int inode_free (super_block_t *sb, int index);
char* path_get_component (char *components, int index);
int path_explode (char *path, char *components) ;
int fs_read (fs_t *fs, int index, int offset, void *buffer, int len);
int fs_write (fs_t *fs, int index, int offset, void *buffer, int len) ;
int fs_append (fs_t *fs, int index, void *buffer, int len);

int inode_append (super_block_t *sb, int index, void *buffer, int len);

int inode_write (super_block_t *sb, int index, int offset, void *buffer, int len);
int inode_read (super_block_t *sb, int index, int offset, void *buffer, int len);
int inode_resize (super_block_t *sb, int index, int size) ;
int inode_shrink (super_block_t	*sb, int index, int size);
int inode_shrink_1_level (super_block_t *sb, void **p);
int inode_expand (super_block_t *sb, int index, int size);

void *fs_allocate_block (fs_t *fs);
int fs_free_block (fs_t *fs, int free_block_number);
int fs_addr_to_block_number (fs_t *fs, void *free_block_addr);
void* loc_locate (location_t *location, int offset);

int loc_index (location_t *location, int offset, location_index_t *index);

#endif