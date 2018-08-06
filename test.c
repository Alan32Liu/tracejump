#include <unistd.h>
#include <stdio.h>

unsigned int test(unsigned int x) {
    if(x < 128) {
        if(x < 64) {
            if(x < 32) {
                return 0;
            } else {
                return 1;
            }
        } else {
            if(x < 64 + 32) {
                return 2;
            } else {
                return 3;
            }
        }
    } else {
        if(x < 128 + 64) {
            if(x < 128 + 32) {
                return 4;
            } else {
                return 5;
            }
        } else {
            if(x < 128 + 64 + 32) {
                return 6;
            } else {
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
