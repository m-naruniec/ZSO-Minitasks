//compilation: gcc -O0 -nostdlib -o custom_threads_hard custom_threads_hard.c
#define _GNU_SOURCE
#include <unistd.h>
#include <wait.h>
#include <sched.h>
#include <sys/mman.h>
#include <stdint.h>
#include <syscall.h>

#define STACK_SIZE (4 * 1024 * 1024) // 4 MB
#define WRITE_TIMES 1000

void my_exit() {
	__asm__ (
		"movq %0, %%rdi;"
		"movq %1, %%rax;"
		"syscall;"
		:
		: "r" (0LL), "r" ((uint64_t)SYS_exit)
		: "%rdi", "%rax", "%rcx", "%r11"
	);
}

void my_write(long long fd, const char *buff, long long len) {
	__asm__ (
		"syscall;"
		:
		: "D" (fd), "S" ((uint64_t)buff), "d" (len), "a" ((uint64_t)SYS_write)
		: "%rcx", "%r11"
	);
}

void my_wait() {
	__asm__ (
		"movq %%rcx, %%r10;"
		"syscall;"
		:
		: "D" (-1LL), "S" (0LL), "d" (0LL), "c" (0LL),
		  "a" ((uint64_t)SYS_wait4)
		: "%r10", "%r11"
	);
}

void *create_stack() {
	void *res = NULL;
	__asm__ (
			"movq %%rcx, %%r10;"
			"movq %1, %%r8;"
			"movq %2, %%r9;"
			"syscall;"
			: "=a" ((uint64_t)res)
			: "D" (0LL),
			  "S" ((uint64_t)STACK_SIZE),
			  "d" ((uint64_t)(PROT_WRITE | PROT_READ)),
			  "c" ((uint64_t)(MAP_ANONYMOUS | MAP_PRIVATE | MAP_GROWSDOWN)),
			  "r" (-1LL),
			  "r" (0LL),
			  "a" ((uint64_t)SYS_mmap)
			: "%r10", "%r8", "%r9", "%r11"
	);
	return res;
}

void my_clone(int (*fn)(void *), void *child_stack) {
	child_stack += STACK_SIZE - 16;
	*(uint64_t *)(child_stack + 8) = (uint64_t)fn;
	__asm__ (
		"movq %%rcx, %%r10;"
		"syscall;"
		:
		: "D" (SIGCHLD | CLONE_FILES | CLONE_FS | CLONE_IO | CLONE_PTRACE
		   | CLONE_SIGHAND | CLONE_VM),
		  "S" ((uint64_t)child_stack),
		  "d" (1LL),
		  "c" (0LL),
		  "a" ((uint64_t)SYS_clone)
		: "%r10", "%r8", "%r9", "%r11"
	);
}

void create_thread(int (*fn)(void *)) {
	void *child_stack = create_stack();
	my_clone(fn, child_stack);
}

void entry(void) {
	int i;
	for(i = 0; i < WRITE_TIMES; ++i) {
		my_write(2, "A\n", 2);
	}
	my_exit();
}

int _start() {
	create_thread((int (*)(void *))&entry);
	int i;
	for(i = 0; i < WRITE_TIMES; ++i) {
		my_write(1, "B\n", 2);
	}
	my_wait();
	my_exit();
}
