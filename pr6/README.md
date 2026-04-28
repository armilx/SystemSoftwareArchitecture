# Практична робота №6

**Тема**: ІНСТРУМЕНТИ НАЛАГОДЖЕННЯ ДЛЯ ПРОБЛЕМ З ПАМ'ЯТТЮ.

## Варіант 2

**Завдання:** Створити програму з навмисним пошкодженням метаданих купи та порівняти діагностику помилки в glibc, AddressSanitizer і Valgrind з поясненням відмінностей.

## Теоретичні відомості

Пам'ять на купі (heap) виділяється блоками. Перед кожним таким блоком система зберігає службові дані (метадані) — розмір блоку та його статус. Якщо програма виходить за межі свого масиву, вона затирає метадані сусіднього блоку. 
Для виявлення таких помилок існують різні інструменти: стандартний системний аллокатор (glibc), зовнішні аналізатори (Valgrind) та вбудовані в компілятор санітайзери (AddressSanitizer). Всі вони працюють за різними принципами і видають різні результати.

## Хід роботи

Було написано програму мовою C. Функція `malloc` двічі виділяє по 16 байт пам'яті (для масивів `a` та `b`). Цикл `for` навмисно виконує 8 ітерацій запису замість 4 дозволених, виходячи за межі масиву `a` і пошкоджуючи метадані масиву `b`.

### Код програми (corruption.c)

```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    int *a = malloc(16);
    int *b = malloc(16);

    for (int i = 0; i < 8; i++) {
        a[i] = 0;
    }

    free(b);
    free(a);

    return 0;
}
```

### 1. Звичайна компіляція та запуск (діагностика glibc)

При звичайній компіляції за цілісність пам'яті відповідає стандартна бібліотека `glibc`.

```bash
andriib@andriib:~$ gcc corruption.c -o corr
andriib@andriib:~$ ./corr
free(): invalid pointer
Aborted (core dumped)

```
**Пояснення:** Програма спокійно виконує вихід за межі масиву і не падає під час запису. Помилка виникає лише в кінці, коли функція `free(b)` намагається прочитати зіпсований заголовок блоку.

### 2. Діагностика через Valgrind

Запускаємо нашу звичайну скомпільовану програму через зовнішній аналізатор пам'яті.

```bash
andriib@andriib:~$ valgrind ./corr
==30440== Memcheck, a memory error detector
==30440== Copyright (C) 2002-2022, and GNU GPL'd, by Julian Seward et al.
==30440== Using Valgrind-3.22.0 and LibVEX; rerun with -h for copyright info
==30440== Command: ./corr
==30440== 
==30440== Invalid write of size 4
==30440==    at 0x108810: main (in /home/andriib/corr)
==30440==  Address 0x4a7e050 is 0 bytes after a block of size 16 alloc'd
==30440==    at 0x4885250: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-arm64-linux.so)
==30440==    by 0x1087E7: main (in /home/andriib/corr)
==30440== 
==30440== 
==30440== HEAP SUMMARY:
==30440==     in use at exit: 0 bytes in 0 blocks
==30440==   total heap usage: 2 allocs, 2 frees, 32 bytes allocated
==30440== 
==30440== All heap blocks were freed -- no leaks are possible
==30440== 
==30440== For lists of detected and suppressed errors, rerun with: -s
==30440== ERROR SUMMARY: 4 errors from 1 contexts (suppressed: 0 from 0)

```
**Пояснення:** Valgrind перехоплює помилку безпосередньо **в момент запису**. Він чітко вказує рядок коду (9), де стався нелегальний доступ, і повідомляє, що запис відбувся одразу після виділеного 16-байтного блоку.

### 3. Діагностика через AddressSanitizer (ASan)

Для перевірки через ASan програму потрібно перекомпілювати зі спеціальним прапорцем `-fsanitize=address`.

```bash
andriib@andriib:~$ gcc -fsanitize=address -g corruption.c -o corr_asan
andriib@andriib:~$ ./corr_asan
=================================================================
==30458==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x502000000020 at pc 0xb6404e0609a4 bp 0xffffefcf0e80 sp 0xffffefcf0e70
WRITE of size 4 at 0x502000000020 thread T0
    #0 0xb6404e0609a0 in main /home/andriib/corruption.c:9
    #1 0xfb19fdb784c0 in __libc_start_call_main ../sysdeps/nptl/libc_start_call_main.h:58
    #2 0xfb19fdb78594 in __libc_start_main_impl ../csu/libc-start.c:360
    #3 0xb6404e06082c in _start (/home/andriib/corr_asan+0x82c) (BuildId: c1a188e5bab52ad11fcccffe5b1d5ba973200545)

0x502000000020 is located 0 bytes after 16-byte region [0x502000000010,0x502000000020)
allocated by thread T0 here:
    #0 0xfb19fddf76d0 in malloc ../../../../src/libsanitizer/asan/asan_malloc_linux.cpp:69
    #1 0xb6404e060924 in main /home/andriib/corruption.c:5
    #2 0xfb19fdb784c0 in __libc_start_call_main ../sysdeps/nptl/libc_start_call_main.h:58
    #3 0xfb19fdb78594 in __libc_start_main_impl ../csu/libc-start.c:360
    #4 0xb6404e06082c in _start (/home/andriib/corr_asan+0x82c) (BuildId: c1a188e5bab52ad11fcccffe5b1d5ba973200545)

SUMMARY: AddressSanitizer: heap-buffer-overflow /home/andriib/corruption.c:9 in main
Shadow bytes around the buggy address:
  0x501ffffffd80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x501ffffffe00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x501ffffffe80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x501fffffff00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x501fffffff80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
=>0x502000000000: fa fa 00 00[fa]fa 00 00 fa fa fa fa fa fa fa fa
  0x502000000080: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x502000000100: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x502000000180: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x502000000200: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x502000000280: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07 
  Heap left redzone:       fa
  Freed heap region:       fd
  Stack left redzone:      f1
  Stack mid redzone:       f2
  Stack right redzone:     f3
  Stack after return:      f5
  Stack use after scope:   f8
  Global redzone:          f9
  Global init order:       f6
  Poisoned by user:        f7
  Container overflow:      fc
  Array cookie:            ac
  Intra object redzone:    bb
  ASan internal:           fe
  Left alloca redzone:     ca
  Right alloca redzone:    cb
==30458==ABORTING

```
**Пояснення:** ASan також ловить помилку в момент запису. Він створює спеціальні захисні зони навколо виділених блоків. Як тільки цикл намагається записати дані в цю зону, програма миттєво зупиняється з помилкою `heap-buffer-overflow`.

## Висновок

Аналіз показав суттєві відмінності у діагностиці пошкодження метаданих купи:
1. **glibc** працює найшвидше, але його діагностика найменш інформативна. Він повідомляє про помилку занадто пізно (під час виклику `free`), що ускладнює пошук місця багу в коді.
2. **Valgrind** є дуже потужним інструментом, який не вимагає перекомпіляції коду. Він точно знаходить місце виходу за межі пам'яті, але сильно уповільнює виконання програми.
3. **AddressSanitizer (ASan)** працює набагато швидше за Valgrind і дає дуже детальний звіт про вихід за межі буфера (`heap-buffer-overflow`), проте вимагає перекомпіляції програми з відповідними прапорцями. 

Для розробки та налагодження найкраще підходять ASan та Valgrind, оскільки вони локалізують проблему саме там, де вона виникла, а не там, де вона призвела до фатальних наслідків.
