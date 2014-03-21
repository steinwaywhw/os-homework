#define _KERNEL_MODE

#include "fs_syscall.h"

#ifndef _KERNEL_MODE
	#include <sys/types.h>
	#include <unistd.h>
#else
	#include <linux/stddef.h>
	#include <linux/sched.h>
	#include <linux/module.h>         /* Needed for the macros */
	MODULE_LICENSE("GPL");

	void dummy (int anything) {}
	int dummy_getpid () {
		int ret = current->pid;
		return ret;
	}
#endif

#include "config.h"


file_t g_file_table[MAX_PROCESS][MAX_OPENFILE];
fs_t *g_fs = NULL;


int sys_creat (char *pathname) {
	int status = fs_mk (g_fs, pathname, INODE_TYPE_REG);
	return status;
}

int sys_mkdir (char *pathname) {
	int status = fs_mk (g_fs, pathname, INODE_TYPE_DIR);
	return status;
}

int sys_open (char *pathname) {
	int inode = inode_lookup_full (g_fs->super_block, pathname);
	if (inode < 0)
		return -1;

	int pid = getpid ();

	// already opened
	int fd = _table_lookup_inode (pid, inode);
	if (fd >= 0) {
		return -1;
	}

	// open
	fd = _table_assign_fd (pid);
	int index = _table_lookup_pid (pid);
	if (index < 0)
		return -1;

	g_file_table[index][fd].pid = pid;
	g_file_table[index][fd].inode = inode;
	g_file_table[index][fd].offset = 0;

	return fd;
}

int sys_close (int fd) {
	int pid = getpid ();
	int status = _table_loolup_fd (pid, fd);
	if (status < 0)
		return -1;

	_table_release_fd (pid, fd);

	return 0;
}

int sys_read (int fd, char *buffer, int len) {
	int pid = getpid ();
	int status = _table_loolup_fd (pid, fd);
	if (status < 0)
		return -1;

	int index = _table_lookup_pid (pid);
	int inode = g_file_table[index][fd].inode;
	int offset = g_file_table[index][fd].offset;

	status = fs_read (g_fs, inode, offset, buffer, len);
	if (status < 0)
		return -1;

	g_file_table[index][fd].offset += status;

	return status;
}

int sys_write (int fd, char *buffer, int len) {
	int pid = getpid ();
	int status = _table_loolup_fd (pid, fd);
	if (status < 0)
		return -1;

	int index = _table_lookup_pid (pid);
	int inode = g_file_table[index][fd].inode;
	int offset = g_file_table[index][fd].offset;

	status = fs_write (g_fs, inode, offset, buffer, len);
	if (status < 0) {
		return -1;
	}

	g_file_table[index][fd].offset += status;

	return status;
}

int sys_lseek (int fd, int offset) {
	int pid = getpid ();
	int status = _table_loolup_fd (pid, fd);
	if (status < 0)
		return -1;

	int index = _table_lookup_pid (pid);
	int inode = g_file_table[index][fd].inode;
	void *location = loc_locate (&g_fs->super_block->inodes[inode].location, offset);

	if (location == NULL)
		return -1;
	if (inode_isdir (g_fs->super_block, inode))
		return -1;

	g_file_table[index][fd].offset = offset;

	return 0;
}

int sys_unlink (char *pathname) {
	int inode = inode_lookup_full (g_fs->super_block, pathname);
	if (inode < 0)
		return -1;

	int pid = getpid ();

	// already opened
	int fd = _table_lookup_inode (pid, inode);
	printk ("sys_unlink:%d,%s\n", fd, pathname);

	if (fd >= 0) {
		return -1;
	}

	int status = fs_rm (g_fs, pathname);
	return status;
}

int sys_readdir (int fd, char *buffer) {
	return sys_read (fd, buffer, sizeof (dir_entry_t));
}


int _table_lookup_pid (int pid) {
	int i, j;
	for (i = 0; i < MAX_PROCESS; i++) {
		for (j = 0; j < MAX_OPENFILE; j++) {
			file_t f = g_file_table[i][j];
			if (f.pid == pid)
				return i;
		}
	}
	return -1;
}

int _table_loolup_fd (int pid, int fd) {
	int i = _table_lookup_pid (pid);
	if (i < 0)
		return -1;

	file_t *f = &g_file_table[i][fd];
	if (f->pid == pid)
		return i;

	return -1;
}

int _table_lookup_inode (int pid, int inode) {
	int j;
	int i = _table_lookup_pid (pid);
	if (i < 0)
		return -1;
	
	for (j = 0; j < MAX_OPENFILE; j++) {
		file_t f = g_file_table[i][j];
		if (f.inode == inode)
			return j;
	}
	return -1;
}

int _table_assign_fd (int pid) {
	int i = _table_lookup_pid (pid);
	int j;

	if (i < 0) {
		int counter;
		for (i = 0; i < MAX_PROCESS; i++) {
			counter = 0;
			for (j = 0; j < MAX_OPENFILE; j++) {
				counter += g_file_table[i][j].pid;
				if (counter > 0)
					break;
			}
			if (counter == 0)
				break;
		}
	}

	if (i == MAX_PROCESS)
		return -1;

	for (j = 0; j < MAX_OPENFILE; j++) {
		file_t f = g_file_table[i][j];
		if (f.pid == 0) {
			g_file_table[i][j].pid = pid;
			return j;
		}
	}

	return -1;
}

int _table_release_fd (int pid, int fd) {
	int i = _table_lookup_pid (pid);
	g_file_table[i][fd].pid = 0;
	g_file_table[i][fd].offset = 0;
	g_file_table[i][fd].inode = -1;
	return 0;
}