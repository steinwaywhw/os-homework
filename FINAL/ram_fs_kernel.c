#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/errno.h> 		/* error codes */
#include <linux/proc_fs.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include "fs_syscall.h"

MODULE_LICENSE("GPL");

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

static long proc_ioctl (struct file *file, unsigned int cmd, unsigned long arg);
static struct file_operations proc_ops;
static struct proc_dir_entry *proc_entry;

void printk_tty (char *string) {
	struct tty_struct *tty;
	tty = current->signal->tty;

	if (tty != NULL) {
		(*tty->driver->ops->write)(tty, string, strlen (string));
		(*tty->driver->ops->write)(tty, "\015\012", 2);
	}
} 

static int __init initialization_routine(void) {
	printk ("Loading RAM Disk.\n");

	g_fs = init_fs ();
	memset (g_file_table, 0, sizeof (g_file_table));

	if (g_fs == NULL) {
		printk ("ERROR: init_fs() fails.\n");
		return 1;
	}

	/* Start create proc entry */
	proc_entry = create_proc_entry (PROC_ENTRY, 0666, NULL);
	if(!proc_entry) {
		printk ("ERROR: create_proc_entry() fails.\n");
		return 1;
	}

	proc_ops.unlocked_ioctl = proc_ioctl;
	proc_entry->proc_fops = &proc_ops;

	return 0;
}


// static int do_copy_to_user (char *dest, char *src, int len) {
		
// 	int remained = copy_to_user (dest, src, sizeof (int));

// 	if (remained > 0)
// 		return 

// }


static void __exit cleanup_routine(void) {

	printk ("Unloading RAM Disk.\n");
	remove_proc_entry(PROC_ENTRY, NULL);
	destroy_fs (g_fs);
	return;
}

static long proc_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	
	command_t command;
	char path[MAX_FILE_FULL];
	void *buffer = NULL;
	int status;

	switch (cmd){
		case IOC_OPEN:
			printk ("OPEN\n");
			copy_from_user (&command, (command_t *)arg, sizeof (command_t));
			memset (path, 0, MAX_FILE_FULL);
			copy_from_user (path, command.pathname, command.len);
			status = sys_open (path);
			printk ("%d:%s\n", status, path);
			copy_to_user (command.status, &status, sizeof (int));
			return 0;

		case IOC_CLOSE:
			printk ("CLOSE\n");
			copy_from_user (&command, (command_t *)arg, sizeof (command_t));
			status = sys_close (command.fd);
			copy_to_user (command.status, &status, sizeof (int));
			return 0;


		case IOC_CREAT:
			printk ("CREAT\n");
			copy_from_user (&command, (command_t *)arg, sizeof (command_t));
			memset (path, 0, MAX_FILE_FULL);
			copy_from_user (path, command.pathname, command.len);
			status = sys_creat (path);
			printk ("%d:%s\n", status, path);
			copy_to_user (command.status, &status, sizeof (int));
			return 0;


		case IOC_MKDIR:
			printk ("MKDIR\n");
			copy_from_user (&command, (command_t *)arg, sizeof (command_t));
			memset (path, 0, MAX_FILE_FULL);
			copy_from_user (path, command.pathname, command.len);
			status = sys_mkdir (path);
			printk ("%d:%s\n", status, path);
			copy_to_user (command.status, &status, sizeof (int));
			return 0;


		case IOC_READ:
			printk ("READ\n");
			copy_from_user (&command, (command_t *)arg, sizeof (command_t));

			buffer = vmalloc (command.len);
			memset (buffer, 0, command.len);

			status = sys_read (command.fd, buffer, command.len);
			printk ("%d\n", status);
			copy_to_user (command.buffer, buffer, command.len);

			vfree (buffer);
			buffer = NULL;

			copy_to_user (command.status, &status, sizeof (int));
			return 0;

		case IOC_WRITE:
			printk ("WRITE\n");
			copy_from_user (&command, (command_t *)arg, sizeof (command_t));

			buffer = vmalloc (command.len);
			memset (buffer, 0, command.len);
			copy_from_user (buffer, command.buffer, command.len);

			status = sys_write (command.fd, buffer, command.len);
			printk ("%d\n", status);
			vfree (buffer);
			buffer = NULL;

			copy_to_user (command.status, &status, sizeof (int));
			return 0;

		case IOC_LSEEK:
			printk ("LSEEK\n");
			copy_from_user (&command, (command_t *)arg, sizeof (command_t));
			status = sys_lseek (command.fd, command.len);
			copy_to_user (command.status, &status, sizeof (int));
			printk ("%d:%d\n", status, command.len);
			return 0;


		case IOC_UNLINK:
			printk ("UNLINK\n");
			copy_from_user (&command, (command_t *)arg, sizeof (command_t));
			memset (path, 0, MAX_FILE_FULL);
			copy_from_user (path, command.pathname, command.len);
			status = sys_unlink (path);
			copy_to_user (command.status, &status, sizeof (int));
			printk ("%d:%s\n", status, path);
			return 0;


		case IOC_READDIR:
			printk ("READDIR\n");
			copy_from_user (&command, (command_t *)arg, sizeof (command_t));
			if (buffer != NULL)
				vfree (buffer);

			buffer = vmalloc (sizeof (dir_entry_t));
			memset (buffer, 0, sizeof (dir_entry_t));

			status = sys_readdir (command.fd, buffer);
			copy_to_user (command.buffer, buffer, sizeof (dir_entry_t));

			vfree (buffer);
			buffer = NULL;
			printk ("%d\n", status);
			copy_to_user (command.status, &status, sizeof (int));
			return 0;

		default:
			return -EINVAL;
	}

	return -EINVAL;
}

module_init(initialization_routine); 
module_exit(cleanup_routine); 