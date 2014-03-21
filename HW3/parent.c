#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include "syscall_table.h"

typedef int bool;
#define true 1
#define false 0


void do_child();
void do_parent(pid_t child);
void do_write(pid_t child, struct user_regs_struct *regs);
void debug_syscall(int syscall, bool is_enter);
void copy_from_child(pid_t child, void *src, void *dest, int len);
void copy_to_child(pid_t child, void *src, void *dest, int len);
void translate(char *str);

int main()
{
	pid_t child;
	child = fork();

	if (child == 0)
		do_child();
	else 
		do_parent(child);
}

inline void do_child()
{
	ptrace(PTRACE_TRACEME, 0, NULL, NULL);
	execv("./child", NULL);
}

inline void do_parent(pid_t child)
{
	struct user_regs_struct regs;
	bool enter_syscall;
	int status;

	enter_syscall = false;
	while (1) 
	{
		wait(&status);
		if (WIFEXITED(status))
			break;

		ptrace(PTRACE_GETREGS, child, NULL, &regs);
		//debug_syscall(regs.orig_eax, enter_syscall);

		if (enter_syscall) 
		{
			enter_syscall = false;
			if (regs.orig_eax == SYS_write)
				do_write(child, &regs);
		} 
		else 
		{
			enter_syscall = true;
		}

		ptrace(PTRACE_SYSCALL, child, NULL, NULL);
	}

	putchar('\n');
}

void do_write(pid_t child, struct user_regs_struct *regs)
{
	int str_len = regs->edx;
	void *str_addr = (void *)regs->ecx;

	char *buffer;
	buffer = (char *)malloc((str_len + 1) * sizeof(char));

	copy_from_child(child, str_addr, buffer, str_len);
	buffer[str_len] = '\0';

	printf ("\nOriginal: %s\n", buffer);
	translate(buffer);

	copy_to_child(child, buffer, str_addr, str_len);

	free(buffer);
}

void translate(char *str)
{
	int len;
	int i;
	len = strlen(str);
	for (i = 0; i < len; i++)
	{
		if (str[i] >= 'a' && str[i] <= 'z')
			str[i] = 'a' + 'z' - str[i];
		else if (str[i] >= 'A' && str[i] <= 'Z')
			str[i] = 'A' + 'Z' - str[i];
	}
}

void copy_from_child(pid_t child, void *src, void *dest, int len)
{
	int i;

	for (i = 0; i < len; i += 4)	
	{
		int to_copy = len - i > 4 ? 4 : len - i;
		long data = ptrace(PTRACE_PEEKDATA, child, (unsigned int)src + i, NULL);
		memcpy(dest + i, &data, to_copy);
	}
}

void copy_to_child(pid_t child, void *src, void *dest, int len)
{
	int i;

	for (i = 0; i < len; i += 4)	
	{
		int to_copy = len - i > 4 ? 4 : len - i;
		long data;
		memcpy(&data, src + i, to_copy);
		ptrace(PTRACE_POKEDATA, child, (unsigned int)dest + i, data);
	}
}


inline void debug_syscall(int syscall, bool is_enter)
{
	if (is_enter)
		printf("Entering syscall: %-20s\t", syscall_table[syscall]);
	else
		printf("Leaving syscall: %-20s\n", syscall_table[syscall]);
}