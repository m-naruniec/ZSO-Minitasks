#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>

#define FORMAT_OFFSET 8
#define PRINTF_OFFSET 18
#define TEMPLATE_LEN 30 // not counting null byte

const char* template =
"\x55" // push %rbp
"\x48\x89\xe5" // mov %rsp,%rbp
"\x89\xfe" // mov %edi,%esi
"\x48\xbf\x88\x77\x66\x55\x44\x33\x22\x11" // movabs $0x1122334455667788,%rdi
"\x48\xb9\x88\x77\x66\x55\x44\x33\x22\x11" // movabs $0x1122334455667788,%rcx
"\xff\xd1" // callq *%rcx
"\x5d" // pop %rbp
"\xc3"; // ret

typedef void (*formatter) (int);

formatter make_formatter (const char *format) {
    formatter result = mmap(NULL, TEMPLATE_LEN,
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    memcpy(result, template, TEMPLATE_LEN);
    *(uint64_t *)(result + FORMAT_OFFSET) = (uint64_t)format;
    *(uint64_t *)(result + PRINTF_OFFSET) = (uint64_t)printf;

    return result;
}

int main() {
    formatter x08_format = make_formatter ("%08x\n");
    formatter xalt_format = make_formatter ("%#x\n");
    formatter d_format = make_formatter ("%d\n");
    formatter verbose_format = make_formatter ("Liczba: %9d!\n");

    x08_format (0x1234);
    xalt_format (0x5678);
    d_format (0x9abc);
    verbose_format (0xdef0);

    return 0;
}
