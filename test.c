#include <unistd.h>
#include <stdio.h>

void __trace_jump();

unsigned int test(unsigned int x) {
    __trace_jump();
    if(x < 128) {
        __trace_jump();
        if(x < 64) {
            __trace_jump();
            if(x < 32) {
                __trace_jump();
                return 0;
            } else {
                __trace_jump();
                return 1;
            }
        } else {
            __trace_jump();
            if(x < 64 + 32) {
                __trace_jump();
                return 2;
            } else {
                __trace_jump();
                return 3;
            }
        }
    } else {
        __trace_jump();
        if(x < 128 + 64) {
            __trace_jump();
            if(x < 128 + 32) {
                __trace_jump();
                return 4;
            } else {
                __trace_jump();
                return 5;
            }
        } else {
            __trace_jump();
            if(x < 128 + 64 + 32) {
                __trace_jump();
                return 6;
            } else {
                __trace_jump();
                return 7;
            }
        }
    }
}

int main(int argc, char * argv[]) {
    unsigned char x;
    read(0, &x, 1);
    unsigned int r = test(x);
    return r;
}
