#include <sys/mman.h>
#include <string.h>

#define ALLOC_SIZE (1024 * 1024)
#define ITERATIONS 10000

int main() {
    for (int i = 0; i < ITERATIONS; i++) {

        void* ptr = mmap(NULL, ALLOC_SIZE, 
                         PROT_READ | PROT_WRITE, 
                         MAP_PRIVATE | MAP_ANONYMOUS, 
                         -1, 0);
        memset(ptr, 1, ALLOC_SIZE);

        munmap(ptr, ALLOC_SIZE);
    }
    return 0;
}
