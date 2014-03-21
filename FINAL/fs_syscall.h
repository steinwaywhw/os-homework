#ifndef _FS_SYSCALL_H
#define _FS_SYSCALL_H

#include "fs.h"
#include "config.h"



int sys_creat (char *pathname);
int sys_mkdir (char *pathname);
int sys_open (char *pathname);
int sys_close (int fd);
int sys_read (int fd, char *buffer, int len);
int sys_write (int fd, char *buffer, int len);
int sys_lseek (int fd, int offset);
int sys_unlink (char *pathname);
int sys_readdir (int fd, char *buffer);

typedef struct file_t {
	int inode;
	int pid;
	int offset;
} file_t;


extern file_t g_file_table[MAX_PROCESS][MAX_OPENFILE];
extern fs_t *g_fs;

int _table_lookup_pid (int pid);
int _table_assign_fd (int pid);
int _table_release_fd (int pid, int fd);
int _table_lookup_inode (int pid, int inode);
int _table_loolup_fd (int pid, int fd);

#endif