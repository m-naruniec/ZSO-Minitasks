#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>
#include <sched.h>
#include <sys/mman.h>
//#include <unitypes.h>
#include <stdint.h>
#include <syscall.h>
#include <stdio.h>
#include <string.h>
#include <asm/errno.h>

#define STACK_SIZE (4 * 1024 * 1024) // 4 MB

int my_exit() {
	__asm__ (
			"movq %0, %%rdi;"
			"movq %1, %%rax;"
			"syscall;"
	:
	: "r" (0LL), "r" ((long long)SYS_exit)
	: "%rdi", "%rax", "%rcx", "%r11"
	);
}

int my_write(long long fd, const char *buff, long long len) {
	__asm__ (
		"movq %0, %%rdi;"
		"movq %1, %%rsi;"
		"movq %2, %%rdx;"
		"movq %3, %%rax;"
		"syscall;"
		:
		: "r" (fd), "r" ((long long)buff), "r" (len), "r" ((long long)SYS_write)
		: "%rdi", "%rsi", "%rdx", "%rax", "%rcx", "%r11"
	);
}

void my_clone() {
}


int entry(void) {
    for(int i = 0; i < 1000; ++i) {
        //write(1, "A\n", 2);
		my_write(1, "A\n", 2);

	}
    //_exit(0);
	my_exit();
}

int my_wait() {
	__asm__ (
		"movq %0, %%r10;"
		"syscall;"
		:
		: "D" (-1LL), "S" (0LL), "d" (0LL), "r" (0LL), "a" ((long long)SYS_wait4)
		: "%r10", "%rcx", "%r11"
	);
}

void *create_stack() {
	void *res = NULL;
	__asm__ (
			"movq %%rcx, %%r10;"
			"movq %1, %%r8;"
			"movq %2, %%r9;"
			"syscall;"
	: "=a" ((long long)res)
	: "D" (0LL),
	  "S" ((long long)STACK_SIZE),
	  "d" ((long long)(PROT_WRITE | PROT_READ)),
	  "c" ((long long)(MAP_ANONYMOUS | MAP_PRIVATE | MAP_GROWSDOWN)),
	  "r" (-1LL),
	  "r" (0LL),
	  "a" ((long long)SYS_mmap)
	: "%r10", "%r8", "%r9", "%r11"
	);
	printf("%p\n", res); fflush(stdout);
	printf("%s\n", strerror(-(int)res));
	return res;
}

pid_t create_thread(int (*fn)(void *)) {
    //void *child_stack = mmap(NULL, STACK_SIZE, PROT_WRITE | PROT_READ,
      //                       MAP_ANONYMOUS | MAP_PRIVATE | MAP_GROWSDOWN,
        //                     -1, 0);
    void *child_stack = create_stack();

	printf("%p\n", child_stack); fflush(stdout);

	//child_stack += STACK_SIZE - 8;
    //*(uint64_t *)child_stack = (uint64_t)fn;
    return clone(fn, child_stack + STACK_SIZE,
                 SIGCHLD | CLONE_FILES | CLONE_FS | CLONE_IO | CLONE_PTRACE
                 | CLONE_SIGHAND | CLONE_VM, NULL);
}


int main() {
    pid_t child_pid = create_thread((int (*)(void *))&entry);
    for(int i = 0; i < 1000; ++i) {
        write(1, "B\n", 2);
    }
    my_wait();
	sleep(15);
    //exit(0);
	my_exit();
}
/*
asm("mov $9, %%rax;"
		"mov %3, %%r10;"
		"mov %4, %%r8;"
		"mov %5, %%r9;"
		"syscall"
:
: "D" (addr), "S" (length), "d" (prot), "r" (flags), "r" (fd),
"r" (offset)
: "%rax", "%r10", "%r8", "%r9", "%rcx", "%r11"
);
 */