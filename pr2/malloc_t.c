#include <stdlib.h>
#include <string.h>

#define ALLOC_SIZE (1024 * 1024)
#define ITERATIONS 10000

int main() {
    for (int i = 0; i < ITERATIONS; i++) {
        void* ptr = malloc(ALLOC_SIZE);

        memset(ptr, 1, ALLOC_SIZE);

        free(ptr);
    }
    return 0;
}
