#include <stdio.h>

extern "C" void Hello(const char*);

void Hello(const char* name) {
    printf("Hello, %s\n", name);
}
