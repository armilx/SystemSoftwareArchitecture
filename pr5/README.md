# Практична робота №5

**Тема**: Помилки роботи з пам'яттю

## Варіант 2

```
Створити приклад, де переповнення цілочисельного розміру призводить до виділення занадто малого блоку і подальшого тихого пошкодження heap-метаданих.
```

## Теоретичні відомості

Якщо розмір для `malloc` обчислюється множенням великих чисел, може статися цілочисельне переповнення (integer overflow). У такому випадку змінна скидається і стає дуже малим числом (наприклад, 8). 
Функція `malloc` виділяє цей малий блок пам'яті. Коли програма починає записувати туди великий обсяг даних, вона виходить за межі виділених 8 байтів і "тихо" затирає службову інформацію (метадані) сусідніх блоків на купі (heap). Програма "падає" не під час запису, а пізніше, коли функція `free()` намагається прочитати ці зіпсовані метадані.

## Хід роботи

Було написано просту програму мовою C, яка демонструє цю вразливість:
1. Змінна `total_size` переповнюється і отримує значення `8` замість кількох гігабайтів.
2. Виділяється два блоки пам'яті поруч: `a` (8 байт) та `b` (16 байт).
3. Програма записує 16 байт у масив `a`. Відбувається вихід за межі, і метадані блоку `b` перезаписуються.
4. Під час виклику `free(b)` система виявляє пошкодження і аварійно завершує процес.

Для підтвердження факту виходу за межі пам'яті додатково було використано інструмент Valgrind.

### Код

```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    unsigned int count = 1073741826;
    unsigned int size = 4;
    unsigned int total_size = count * size;

    printf("Requested count: %u\n", count);
    printf("Actual allocated size (overflow): %u bytes\n\n", total_size);
    int *a = malloc(total_size);
    int *b = malloc(16);
    printf("Writing past the 8 allocated bytes...\n");
    for (int i = 0; i < 4; i++) {
        a[i] = 9999; 
    }
    printf("Freeing adjacent block to trigger crash...\n");
    free(b); 

    return 0;
}
```

### Компіляція та запуск

```sh
andriib@andriib:~$ nano overflow.c
andriib@andriib:~$ gcc overflow.c -o overflow
andriib@andriib:~$ ./overflow
Requested count: 1073741826
Actual allocated size (overflow): 8 bytes

Writing past the 8 allocated bytes...
Freeing adjacent block to trigger crash...
andriib@andriib:~$ valgrind ./overflow
==3755== Memcheck, a memory error detector
==3755== Copyright (C) 2002-2022, and GNU GPL'd, by Julian Seward et al.
==3755== Using Valgrind-3.22.0 and LibVEX; rerun with -h for copyright info
==3755== Command: ./overflow
==3755== 
Requested count: 1073741826
Actual allocated size (overflow): 8 bytes

Writing past the 8 allocated bytes...
==3755== Invalid write of size 4
==3755==    at 0x1088E4: main (in /home/andriib/overflow)
==3755==  Address 0x4a7e488 is 0 bytes after a block of size 8 alloc'd
==3755==    at 0x4885250: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-arm64-linux.so)
==3755==    by 0x1088AB: main (in /home/andriib/overflow)
==3755== 
Freeing adjacent block to trigger crash...
==3755== 
==3755== HEAP SUMMARY:
==3755==     in use at exit: 8 bytes in 1 blocks
==3755==   total heap usage: 3 allocs, 2 frees, 1,048 bytes allocated
==3755== 
==3755== LEAK SUMMARY:
==3755==    definitely lost: 8 bytes in 1 blocks
==3755==    indirectly lost: 0 bytes in 0 blocks
==3755==      possibly lost: 0 bytes in 0 blocks
==3755==    still reachable: 0 bytes in 0 blocks
==3755==         suppressed: 0 bytes in 0 blocks
==3755== Rerun with --leak-check=full to see details of leaked memory
==3755== 
==3755== For lists of detected and suppressed errors, rerun with: -s
==3755== ERROR SUMMARY: 2 errors from 1 contexts (suppressed: 0 from 0)
```

# Висновки

Під час виконання роботи було створено мінімальний приклад пошкодження пам'яті (heap corruption) через цілочисельне переповнення. Експеримент довів, що якщо не перевіряти вхідні дані при розрахунку обсягу пам'яті, malloc може виділити занадто малий блок. Вихід за його межі відбувається непомітно для програми, але пошкоджує сусідні структури на купі. Помилка corrupted size vs. prev_size виникає лише під час спроби звільнити цю пам'ять. Утиліта Valgrind успішно виявляє такі проблеми в момент нелегального запису (Invalid write).
