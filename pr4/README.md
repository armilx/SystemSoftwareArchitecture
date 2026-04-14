# Практична робота №4

**Тема**: Динамічне виділення пам'яті

## Завдання 4.1

Хоча тип даних size_t дозволяє передати в malloc число до 16 ексабайт (2^64), операційна система Linux завжди ділить весь віртуальний адресний простір навпіл: половину забирає ядро ОС для своїх потреб і захисту, а половину дає програмам користувача. Тому теоретична межа для програми складає половину від 16, тобто 8 ексабайт.

На практиці ж сучасні процесори x86_64 використовують лише 48-бітну адресацію (загалом 256 ТБ). Тому реальний ліміт для malloc становить приблизно 128 Терабайт (половина від 256 ТБ), і з цього обсягу ще віднімається пам'ять, яку вже займає сам код програми, її стек та підключені бібліотеки.

## Завдання 4.2

### Код

```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    int xa = 100000;
    int xb = 50000;
    int num = xa * xb;

    printf("Values: xa = %d, xb = %d\n", xa, xb);
    printf("Result of xa * xb (signed int): %d\n", num);

    printf("What malloc actually sees (unsigned size_t): %zu\n", (size_t)num);
    
    void *ptr = malloc(num);

    if (ptr == NULL) {
        printf("\n=> RESULT: malloc FAILED! Operating system refused to give memory.\n");
    } else {
        printf("\n=> RESULT: malloc SUCCEEDED!\n");
        free(ptr);
    }

    return 0;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ nano malloc_test.c
andriib@andriib:~$ gcc malloc_test.c -o malloc_test
andriib@andriib:~$ ./malloc_test
Values: xa = 100000, xb = 50000
Result of xa * xb (signed int): 705032704
What malloc actually sees (unsigned size_t): 705032704

=> RESULT: malloc SUCCEEDED!
```
**Висновок:**
Передача від'ємного аргументу (або переповнення знакової змінної) у `malloc` призводить до неявного перетворення типу в гігантське беззнакове число. На будь-якій архітектурі (і x86, і x86_64) такий запит буде відхилено операційною системою через брак віртуального адресного простору, і функція поверне `NULL`.

## Завдання 4.3

### Код
```c
#include <stdio.h>
#include <stdlib.h>

int main() {    
    void *ptr_zero = malloc(0);

    if (ptr_zero == NULL) {
        printf("Result: malloc(0) returned NULL.\n");
    } else {
        printf("Result: malloc(0) returned a valid memory address: %p\n", ptr_zero);
        printf("Warning: Dereferencing this pointer will cause a crash!\n");
    }

    free(ptr_zero);
    printf("Memory freed successfully.\n");

    return 0;
}
```
### Компіляція та запуск

```bash
andriib@andriib:~$ nano zero_malloc.c
andriib@andriib:~$ gcc zero_malloc.c -o zero_malloc
andriib@andriib:~$ ./zero_malloc
Result: malloc(0) returned a valid memory address: 0xc7fbf42a52a0
Warning: Dereferencing this pointer will cause a crash!
Memory freed successfully.
andriib@andriib:~$ ltrace ./zero_malloc
__libc_start_main(0xc89834c30858, 1, 0xfffffd099928, 0 <unfinished ...>
malloc(0)                                        = 0xc8986991f2a0
printf("Result: malloc(0) returned a val"..., 0xc8986991f2a0Result: malloc(0) returned a valid memory address: 0xc8986991f2a0
) = 66
puts("Warning: Dereferencing this poin"...Warning: Dereferencing this pointer will cause a crash!
)      = 56
free(0xc8986991f2a0)                             = <void>
puts("Memory freed successfully."Memory freed successfully.
)               = 27
__cxa_finalize(0xc89834c50008, 0xc89834c30800, 1, 568) = 1
+++ exited (status 0) +++
```
Згідно зі стандартом мови С, поведінка `malloc(0)` залежить від реалізації компілятора: він може повернути або `NULL`, або унікальний вказівник. 
На системі Linux (бібліотека glibc) `malloc(0)` повертає **реальну, ненульову адресу** на мінімальний блок пам'яті (chunk). Чому так зроблено? Щоб програми не "падали" з помилкою, якщо змінна, яка передається в `malloc`, випадково дорівнюватиме нулю в процесі обчислень.
Хоча ця адреса є дійсною, записувати туди будь-які дані суворо заборонено (це призведе до крашу пам'яті, Undefined Behavior). Аналіз через `ltrace` підтвердив, що виклик `malloc(0)` повертає адресу, а виклик `free()` для цієї адреси відпрацьовує абсолютно коректно. Це доводить, що навіть блок нульового розміру реєструється системою і вимагає обов'язкового звільнення для уникнення витоку пам'яті (memory leak).

## Завдання 4.4

Напишімо програму, що демонструє цей випадок:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    void *ptr = NULL;
    int size = 16;

    for (int i = 1; i <= 2; i++) {
        if (!ptr) {
            ptr = malloc(size);
            printf("Loop %d: Memory allocated at %p\n", i, ptr);
        } else {
            printf("Loop %d: Pointer is NOT NULL (%p), skipping malloc!\n", i, ptr);
        }
        memset(ptr, 'A', size);

        free(ptr);
        printf("Loop %d: Memory freed.\n", i);
    }

    return 0;
}
```
### Компіляція та запуск
```bash 
andriib@andriib:~$ nano bad_ptr.c
andriib@andriib:~$ gcc bad_ptr.c -o bad_ptr
andriib@andriib:~$ valgrind ./bad_ptr
==4798== Memcheck, a memory error detector
==4798== Copyright (C) 2002-2022, and GNU GPL'd, by Julian Seward et al.
==4798== Using Valgrind-3.22.0 and LibVEX; rerun with -h for copyright info
==4798== Command: ./bad_ptr
==4798== 
Loop 1: Memory allocated at 0x4a7e040
Loop 1: Memory freed.
Loop 2: Pointer is NOT NULL (0x4a7e040), skipping malloc!
==4798== Invalid write of size 8
==4798==    at 0x4891F9C: memset (in /usr/libexec/valgrind/vgpreload_memcheck-arm64-linux.so)
==4798==    by 0x1088CF: main (in /home/andriib/bad_ptr)
==4798==  Address 0x4a7e040 is 0 bytes inside a block of size 16 free'd
==4798==    at 0x48882A8: free (in /usr/libexec/valgrind/vgpreload_memcheck-arm64-linux.so)
==4798==    by 0x1088D7: main (in /home/andriib/bad_ptr)
==4798==  Block was alloc'd at
==4798==    at 0x4885250: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-arm64-linux.so)
==4798==    by 0x10888B: main (in /home/andriib/bad_ptr)
==4798== 
==4798== Invalid free() / delete / delete[] / realloc()
==4798==    at 0x48882A8: free (in /usr/libexec/valgrind/vgpreload_memcheck-arm64-linux.so)
==4798==    by 0x1088D7: main (in /home/andriib/bad_ptr)
==4798==  Address 0x4a7e040 is 0 bytes inside a block of size 16 free'd
==4798==    at 0x48882A8: free (in /usr/libexec/valgrind/vgpreload_memcheck-arm64-linux.so)
==4798==    by 0x1088D7: main (in /home/andriib/bad_ptr)
==4798==  Block was alloc'd at
==4798==    at 0x4885250: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-arm64-linux.so)
==4798==    by 0x10888B: main (in /home/andriib/bad_ptr)
==4798== 
Loop 2: Memory freed.
==4798== 
==4798== HEAP SUMMARY:
==4798==     in use at exit: 0 bytes in 0 blocks
==4798==   total heap usage: 2 allocs, 3 frees, 1,040 bytes allocated
==4798== 
==4798== All heap blocks were freed -- no leaks are possible
==4798== 
==4798== For lists of detected and suppressed errors, rerun with: -s
==4798== ERROR SUMMARY: 3 errors from 2 contexts (suppressed: 0 from 0)
```

Щоб виправити помилку, достатньо просто додати ptr = NULL; одразу після free(ptr);.

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    void *ptr = NULL;
    int size = 16;

    for (int i = 1; i <= 2; i++) {
        if (!ptr) {
            ptr = malloc(size);
            printf("Loop %d: Memory allocated at %p\n", i, ptr);
        }

        memset(ptr, 'A', size);

        free(ptr);
        printf("Loop %d: Memory freed.\n", i);
        
        ptr = NULL;
    }
    return 0;
}
```
Виконаймо повторно:
```bash 
andriib@andriib:~$ nano good_ptr.c
andriib@andriib:~$ gcc good_ptr.c -o good_ptr
andriib@andriib:~$ valgrind ./good_ptr
==4848== Memcheck, a memory error detector
==4848== Copyright (C) 2002-2022, and GNU GPL'd, by Julian Seward et al.
==4848== Using Valgrind-3.22.0 and LibVEX; rerun with -h for copyright info
==4848== Command: ./good_ptr
==4848== 
Loop 1: Memory allocated at 0x4a7e040
Loop 1: Memory freed.
Loop 2: Memory allocated at 0x4a7e4d0
Loop 2: Memory freed.
==4848== 
==4848== HEAP SUMMARY:
==4848==     in use at exit: 0 bytes in 0 blocks
==4848==   total heap usage: 3 allocs, 3 frees, 1,056 bytes allocated
==4848== 
==4848== All heap blocks were freed -- no leaks are possible
==4848== 
==4848== For lists of detected and suppressed errors, rerun with: -s
==4848== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```
Я перевірив виправлений варіант (good_ptr.c) через valgrind. Аналізатор не виявив жодного витоку пам'яті чи помилок доступу (ERROR SUMMARY: 0 errors), що підтверджує правильність рішення.

## Завдання 4.5

```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main() {
    void *original_ptr = malloc(128);
    if (original_ptr == NULL) {
        printf("Initial malloc failed!\n");
        return 1;
    }
    printf("Step 1: Allocated 128 bytes at %p\n", original_ptr);

    size_t impossible_size = SIZE_MAX;
    printf("Step 2: Asking realloc for %zu bytes...\n", impossible_size);
    void *temp_ptr = realloc(original_ptr, impossible_size);

    if (temp_ptr == NULL) {
        printf("\n=> RESULT: realloc failed and returned NULL.\n");
        printf("=> GOOD NEWS: original_ptr is still safe and valid at %p\n", original_ptr);
        free(original_ptr);
        printf("Original memory freed safely. No memory leaks!\n");
    } else {
        printf("\n=> RESULT: realloc succeeded at %p\n", temp_ptr);
        free(temp_ptr);
    }
    return 0;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ nano realloc_test.c
andriib@andriib:~$ gcc realloc_test.c -o realloc_test
andriib@andriib:~$ ./realloc_test
Step 1: Allocated 128 bytes at 0xb4637b15d2a0
Step 2: Asking realloc for 18446744073709551615 bytes...

=> RESULT: realloc failed and returned NULL.
=> GOOD NEWS: original_ptr is still safe and valid at 0xb4637b15d2a0
Original memory freed safely. No memory leaks!

```
## Завдання 4.6
```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Case 1: realloc(NULL, 64)\n");
    void *ptr1 = realloc(NULL, 64);
    
    if (ptr1 != NULL) {
        printf("Result: Memory allocated successfully at %p (Acts exactly like malloc)\n", ptr1);
        free(ptr1);
    }
    printf("\nCase 2: realloc(valid_ptr, 0)\n");
    
    void *ptr2 = malloc(64);
    printf("Initial memory allocated at %p\n", ptr2);

    void *result_ptr = realloc(ptr2, 0);
    
    printf("Result after realloc(ptr2, 0): %p\n", result_ptr);
    return 0;
}
```
### Компіляція та запуск
```bash
andriib@andriib:~$ nano realloc_edge.c
andriib@andriib:~$ gcc realloc_edge.c -o realloc_edge
andriib@andriib:~$ ./realloc_edge
Case 1: realloc(NULL, 64)
Result: Memory allocated successfully at 0xb26f9536e6b0 (Acts exactly like malloc)

Case 2: realloc(valid_ptr, 0)
Initial memory allocated at 0xb26f9536e6b0
Result after realloc(ptr2, 0): (nil)
```
## Завдання 4.7

### Код

Початковий варіант з realloc:

```c
#include <stdio.h>
#include <stdlib.h>

struct sbar {
    float x;
    float y;
};

int main() {
    struct sbar *ptr, *newptr;

    ptr = calloc(1000, sizeof(struct sbar));
    newptr = realloc(ptr, 500 * sizeof(struct sbar));
    
    if (newptr != NULL) {
        ptr = newptr;
    }

    free(ptr);
    return 0;
}
```
Переписаний варіант з reallocarray:

```c
#include <stdio.h>
#include <stdlib.h>

struct sbar {
    float x;
    float y;
};

int main() {
    struct sbar *ptr, *newptr;

    ptr = calloc(1000, sizeof(struct sbar));
    newptr = reallocarray(ptr, 500, sizeof(struct sbar));
    
    if (newptr != NULL) {
        ptr = newptr;
    }

    free(ptr);
    return 0;
}
```


### Компіляція та запуск

```bash
andriib@andriib:~$ nano old_realloc.c
andriib@andriib:~$ nano new_realloc.c
andriib@andriib:~$ gcc old_realloc.c -o old_realloc
andriib@andriib:~$ gcc new_realloc.c -o new_realloc
andriib@andriib:~$ ltrace ./old_realloc
__libc_start_main(0xb817d00f0818, 1, 0xfffffd357408, 0 <unfinished ...>
calloc(1000, 8)                                  = 0xb817e976d2a0
realloc(0xb817e976d2a0, 4000)                    = 0xb817e976d2a0
free(0xb817e976d2a0)                             = <void>
__cxa_finalize(0xb817d0110008, 0xb817d00f07c0, 1, 568) = 1
+++ exited (status 0) +++
andriib@andriib:~$ ltrace ./new_realloc
__libc_start_main(0xaf7f3b920818, 1, 0xffffd04bd5a8, 0 <unfinished ...>
calloc(1000, 8)                                  = 0xaf7f799ad2a0
reallocarray(0xaf7f799ad2a0, 500, 8, 0xaf7f799af180) = 0xaf7f799ad2a0
free(0xaf7f799ad2a0)                             = <void>
__cxa_finalize(0xaf7f3b940008, 0xaf7f3b9207c0, 1, 568) = 1
+++ exited (status 0) +++
```
Функція reallocarray() робить код безпечнішим. На відміну від звичайного realloc, вона приймає кількість елементів та їхній розмір окремо. Це дозволяє їй перевірити, чи не відбудеться переповнення (overflow) під час їх множення. Якщо запит завеликий, функція поверне NULL (з помилкою ENOMEM), захищаючи програму від неправильного виділення пам'яті.

За допомогою утиліти ltrace я наочно перевірив цю різницю. У класичному варіанті моя програма сама виконала множення, тому ltrace зафіксував виклик realloc(..., 4000). У варіанті з безпечною функцією ltrace показав виклик reallocarray(..., 500, 8). Мій експеримент підтвердив, що в сучасній бібліотеці libc функція reallocarray() викликається напряму як самостійний механізм, а не як обгортка над старим realloc.
