# Завдання за варіантом 14

## Постановка завдання
```
Написати програму з поступовим витоком пам’яті. Підтвердити його через Valgrind, /proc/self/status та власний лічильник алокацій.
```

## Виконання завдання

У ході виконання завдання було написано програму leak.c, яка демонструє поступовий витік пам'яті. 
У циклі 5 разів виділяється по 1 МБ пам'яті без подальшого її звільнення.

### Код
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_memory_status() {
    FILE *file = fopen("/proc/self/status", "r");
    if (!file) return;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            printf("  -> System metric %s", line);
            break;
        }
    }
    fclose(file);
}

int main() {
    int allocation_counter = 0;
    int chunk_size = 1024 * 1024;

    for (int i = 1; i <= 5; i++) {
        void *leak_ptr = malloc(chunk_size);
        
        if (leak_ptr != NULL) {
            memset(leak_ptr, 'A', chunk_size);
            allocation_counter++;
            
            printf("Iteration %d: Highlighted %d МБ (Personal counter: %d)\n", i, chunk_size / (1024 * 1024), allocation_counter);
            
            print_memory_status();
            printf("\n");
        }
        
        sleep(1);
    }

    printf("The work has been completed. A total of: %d МБ.\n", allocation_counter);
    printf("Memory has NOT been freed due to free(), A leak has been detected!\n");

    return 0;
}
```
### Компіляція та запуск

```bash
andriib@andriib:~$ nano leak.c
andriib@andriib:~$ gcc leak.c -o leak
andriib@andriib:~$ ./leak
Iteration 1: Highlighted 1 \u041c\u0411 (Personal counter: 1)
  -> System metric VmRSS:	    2364 kB

Iteration 2: Highlighted 1 \u041c\u0411 (Personal counter: 2)
  -> System metric VmRSS:	    3520 kB

Iteration 3: Highlighted 1 \u041c\u0411 (Personal counter: 3)
  -> System metric VmRSS:	    4548 kB

Iteration 4: Highlighted 1 \u041c\u0411 (Personal counter: 4)
  -> System metric VmRSS:	    5576 kB

Iteration 5: Highlighted 1 \u041c\u0411 (Personal counter: 5)
  -> System metric VmRSS:	    6604 kB

The work has been completed. A total of: 5 \u041c\u0411.
Memory has NOT been freed due to free(), A leak has been detected!

```
Перевірка через Valgrind

```bash
andriib@andriib:~$ valgrind --leak-check=full ./leak
==5624== Memcheck, a memory error detector
==5624== Copyright (C) 2002-2022, and GNU GPL'd, by Julian Seward et al.
==5624== Using Valgrind-3.22.0 and LibVEX; rerun with -h for copyright info
==5624== Command: ./leak
==5624== 
Iteration 1: Highlighted 1 \u041c\u0411 (Personal counter: 1)
  -> System metric VmRSS:	   49468 kB

Iteration 2: Highlighted 1 \u041c\u0411 (Personal counter: 2)
  -> System metric VmRSS:	   50940 kB

Iteration 3: Highlighted 1 \u041c\u0411 (Personal counter: 3)
  -> System metric VmRSS:	   52220 kB

Iteration 4: Highlighted 1 \u041c\u0411 (Personal counter: 4)
  -> System metric VmRSS:	   53524 kB

Iteration 5: Highlighted 1 \u041c\u0411 (Personal counter: 5)
  -> System metric VmRSS:	   54804 kB

The work has been completed. A total of: 5 \u041c\u0411.
Memory has NOT been freed due to free(), A leak has been detected!
==5624== 
==5624== HEAP SUMMARY:
==5624==     in use at exit: 5,242,880 bytes in 5 blocks
==5624==   total heap usage: 16 allocs, 11 frees, 5,251,384 bytes allocated
==5624== 
==5624== 5,242,880 bytes in 5 blocks are definitely lost in loss record 1 of 1
==5624==    at 0x4885250: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-arm64-linux.so)
==5624==    by 0x108BD3: main (in /home/andriib/leak)
==5624== 
==5624== LEAK SUMMARY:
==5624==    definitely lost: 5,242,880 bytes in 5 blocks
==5624==    indirectly lost: 0 bytes in 0 blocks
==5624==      possibly lost: 0 bytes in 0 blocks
==5624==    still reachable: 0 bytes in 0 blocks
==5624==         suppressed: 0 bytes in 0 blocks
==5624== 
==5624== For lists of detected and suppressed errors, rerun with: -s
==5624== ERROR SUMMARY: 1 errors from 1 contexts (suppressed: 0 from 0)

```
