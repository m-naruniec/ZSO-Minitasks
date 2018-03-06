#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>
#include <sched.h>
#include <sys/mman.h>
#include <unitypes.h>

#define STACK_SIZE 4 * 1024 * 1024 // 4 MB

int entry(void) {
    for(int i = 0; i < 1000; ++i) {
        write(1, "A\n", 2);
    }
    _exit(0);
}

pid_t create_thread(int (*fn)(void *)) {
    void *child_stack = mmap(NULL, STACK_SIZE, PROT_WRITE | PROT_READ,
                             MAP_ANONYMOUS | MAP_PRIVATE | MAP_GROWSDOWN,
                             -1, 0);
    child_stack += STACK_SIZE - 8;
    *(uint64_t *)child_stack = (uint64_t)fn;
    return clone(fn, child_stack,
                 SIGCHLD | CLONE_FILES | CLONE_FS | CLONE_IO | CLONE_PTRACE
                 | CLONE_SIGHAND | CLONE_VM, NULL);
}


int main() {
    pid_t child_pid = create_thread((int (*)(void *))&entry);
    for(int i = 0; i < 1000; ++i) {
        write(1, "B\n", 2);
    }
    wait(NULL);
    exit(0);
}