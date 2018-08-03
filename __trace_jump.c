#include <stdio.h>

void __trace_jump() {
    void *rip = __builtin_return_address(0);
    printf("0x%lx\n", (long unsigned int)rip);
}
