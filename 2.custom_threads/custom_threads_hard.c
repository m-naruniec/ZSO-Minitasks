//compilation: gcc -nostdlib -o custom_threads_hard custom_threads_hard.c
#define _GNU_SOURCE
#include <unistd.h>
#include <wait.h>
#include <sched.h>
#include <sys/mman.h>
#include <stdint.h>
#include <syscall.h>

#define STACK_SIZE (4 * 1024) // 4 KB
#define WRITE_TIMES 2

void entry(void);

void my_exit() {
	register uint64_t status_reg asm("rdi") = 1;
	register uint64_t sys_reg asm("rax") = SYS_exit;

	asm volatile (
		"syscall;"
		:
		: "r" (status_reg), "r" (sys_reg)
		: "%rcx", "%r11"
	);
}

void my_write(uint64_t fd, const char *buff, uint64_t len) {
	register uint64_t fd_reg asm("rdi") = fd;
	register uint64_t buff_reg asm("rsi") = (uint64_t)buff;
	register uint64_t len_reg asm("rdx") = len;
	register uint64_t sys_reg asm("rax") = SYS_write;

	asm volatile (
		"syscall;"
		: "=r" (sys_reg)
		: "r" (fd_reg), "r" (buff_reg), "r" (len_reg), "r" (sys_reg)
		: "%rcx", "%r11"
	);
}

void my_wait() {

	asm volatile (
		"movq %%rcx, %%r10;"
		"syscall;"
		:
		: "D" (-1LL), "S" (0LL), "d" (0LL), "c" (0LL),
		  "a" ((uint64_t)SYS_wait4)
		: "%r10", "%r11"
	);
}

void *create_stack() {
	register uint64_t addr_reg asm("rdi") = 0;
	register uint64_t size_reg asm("rsi") = STACK_SIZE;
	register uint64_t prot_reg asm("rdx") = (PROT_WRITE | PROT_READ);
	register uint64_t flags_reg asm("r10") = (MAP_ANONYMOUS | MAP_PRIVATE | MAP_GROWSDOWN);
	register int64_t fd_reg asm("r8") = -1;
	register uint64_t off_reg asm("r9") = 0;
	register uint64_t sys_reg asm("rax") = SYS_mmap;

	asm volatile (
			"syscall;"
			: "=a" (sys_reg)
			: "r" (addr_reg),
			  "r" (size_reg),
			  "r" (prot_reg),
			  "r" (flags_reg),
			  "r" (fd_reg),
			  "r" (off_reg),
			  "r" (sys_reg)
			: "%rcx", "%r11"
	);
	return (void *)sys_reg;
}

void my_clone(int (*fn)(void *), void *child_stack) {
	child_stack += STACK_SIZE - 8;
	*(uint64_t *)child_stack = (uint64_t)fn;

	register uint64_t flags_reg asm("rdi") =
		(SIGCHLD | CLONE_FILES | CLONE_FS | CLONE_IO
		 | CLONE_PTRACE | CLONE_SIGHAND | CLONE_VM);
	register uint64_t stack_reg asm("rsi") = (uint64_t)child_stack;
	register uint64_t ptid_reg asm("rdx") = 1;
	register uint64_t ctid_reg asm("r10") = 0;
	register uint64_t sys_reg asm("rax") = SYS_clone;

	asm volatile (
		"jmp 2f;"
		"1:\n"
			"syscall;"
			"ret;"
		"2:\n"
			"call 1b;"
		: "=a" (sys_reg)
		: "r" (flags_reg),
		  "r" (stack_reg),
		  "r" (ptid_reg),
		  "r" (ctid_reg),
		  "r" (sys_reg)
		: "%rcx", "%r11"
	);

}

void create_thread(int (*fn)(void *)) {
	void *child_stack = create_stack();
	my_clone(fn, child_stack);
}

void entry(void) {
	uint32_t i;
	for(i = 0; i < WRITE_TIMES; ++i) {
		my_write(2, "A\n", 2);
	}
	my_exit();
}

int _start() {
	create_thread((int (*)(void *))&entry);
	uint32_t i;
	for(i = 0; i < WRITE_TIMES; ++i) {
		my_write(1, "B\n", 2);
	}
	my_wait();
	my_exit();
}
