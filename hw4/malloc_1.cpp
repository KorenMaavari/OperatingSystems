#include <stdio.h> // for size_t
#include <cmath> // for pow
#include <unistd.h> // for sbrk

void* smalloc(size_t size) {
    // Check for invalid size
    if (size == 0 || size > pow(10, 8)) {
        return NULL;
    }

    // Call sbrk to increase the program break
    void* result = sbrk(size);

    // Check if sbrk failed
    if (result == (void*)(-1)) {
        return NULL;
    }

    return result;
}
