**Практична робота №2**

Тема: Сегменти виконуваного файлу та організація пам'яті процесу

**Завдання 1**

Визначення моменту переповнення time_t

*Код програми*
```c
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <limits.h>

int main(int argc, char *argv[]) {
    time_t maxtime = 0;

    if (argc < 2) {
        printf("Usage: %s --32bit or %s --64bit\n", argv[0], argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--32bit") == 0) {
        maxtime = (time_t)INT32_MAX;
        printf("Max time_t value is %lld\n", (long long)maxtime);
    } else if (strcmp(argv[1], "--64bit") == 0) {
        maxtime = (time_t)INT64_MAX;
        printf("Max time_t value is %lld\n", (long long)maxtime);
    } else {
        printf("Unknown option\n");
        return 1;
    }

    struct tm *tm = gmtime(&maxtime);
    
    if (!tm) {
        printf("gmtime conversion failed (value too large)\n");
    } else {
        printf("End of time_t (UTC): %s", asctime(tm));
    }

    return 0;
}
```

###Компіляція та запуск

```bash
gcc time.c -o time_test
./time_test --32bit
./time_test --64bit
```

```bash
$ ./time_test --32bit
Max time_t value is 2147483647
End of time_t (UTC): Tue Jan 19 03:14:07 2038

$ ./time_test --64bit
Max time_t value is 9223372036854775807
gmtime conversion failed (value too large)
```

### Висновок
Максимальне значення time_t для 32-бітних систем відповідає даті 19 січня 2038 року,Після цієї дати 32-бітні лічильники часу переповняться. Однак, 64-бітне значення time_t є занадто великим і не може бути конвертоване в дату gmtime(3).

## Завдання 2.2

### Підзавдання 1
Напишемо базовий hello world:

```c
include <stdio.h>

int main(void) {
	puts("Hello World!");
	return 0;
}
```

Виконаємо size:

```bash
$ size helloworld
   text	   data	    bss	    dec	    hex	filename
   1618	    640	      8	   2266	    8da	helloworld

```
### Підзавдання 2

Додамо глобальний масив із 1000 int:

```c
include <stdio.h>

int arr[1000];

int main(void) {
	puts("Hello World!");
	return 0;
}
```

Виконаємо size:

```bash
$ size helloworld
   text	   data	    bss	    dec	    hex	filename
   1618	    640	   4008	   6266	   187a	helloworld

```

### Підзавдання 3

Додамо початкові значення:

```c
include <stdio.h>

int arr[1000] = { 1, 2, 3, 4 };

int main(void) {
	puts("Hello World!");
	return 0;
}
```
Повторимо вимірювання:

```bash
$ size helloworld
   text	   data	    bss	    dec	    hex	filename
   1618	   4640	      8	   6266	   187a	helloworld
```

### Підзавдання 4

```c
#include <stdio.h>

int main(void) {
    int arr[1000] = { 1, 2, 3, 4 };

    puts("Hello World!");
    return 0;
}
```

Повторимо вимірювання:

```bash
$ size helloworld
   text	   data	    bss	    dec	    hex	filename
   2024	    680	      8	   2712	    a98	helloworld
```

## Завдання 2.3

### Код програми

```c
#include <stdio.h>

int my_bss;           
int my_data = 2024;  

int my_func(void) {
    return 2024;
}

int main(void) {
    int stack_var_1; 

    printf("Stack variable address: %p\n", &stack_var_1);
    printf("BSS variable address:   %p\n", &my_bss);
    printf("Data variable address:  %p\n", &my_data);
    printf("Function (Code) address:%p\n", my_func);

    int stack_array[400];
    for (int i = 0; i < 400; i++) {
        stack_array[i] = my_func();
    }

    int stack_var_2;
    printf("(2) Stack variable address: %p\n", &stack_var_2);

    return 0;
}
```

### Компіляція та запуск

```bash
gcc memory_layout.c -o memory_test
./memory_test
```

### Результат виконання

```bash
$ ./memory_test
Stack variable address: 0xffffe954af9c
BSS variable address:   0xc92d0ffb0018
Data variable address:  0xc92d0ffb0010
Function (Code) address:0xc92d0ff90858
(2) Stack variable address: 0xffffe954afa0
```

## Завдання 2.4

### gstack

Спочатку отримаємо PID нашого процесу `bash`:

```bash
$ pidof bash
2541

```

Тепер застосуємо утиліту `gstack (sudo gdb --batch -p 2541 -ex "bt")`:

```bash
$ sudo gdb --batch -p 2541 -ex "bt"

warning: could not find '.gnu_debugaltlink' file for /lib/aarch64-linux-gnu/libtinfo.so.6
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/aarch64-linux-gnu/libthread_db.so.1".
0x0000f7f5a9937b2c in __GI___wait4 (pid=-1, stat_loc=0xffffd387bdc0, options=10, usage=0x0) at ../sysdeps/unix/sysv/linux/wait4.c:30

warning: 30	../sysdeps/unix/sysv/linux/wait4.c: No such file or directory
#0  0x0000f7f5a9937b2c in __GI___wait4 (pid=-1, stat_loc=0xffffd387bdc0, options=10, usage=0x0) at ../sysdeps/unix/sysv/linux/wait4.c:30
#1  0x0000bff17977f984 in ?? ()
#2  0x0000bff1796ccf6c [PAC] in wait_for ()
#3  0x0000bff1796af6a8 [PAC] in execute_command_internal ()
#4  0x0000bff1796afa84 [PAC] in execute_command ()
#5  0x0000bff1796a0800 [PAC] in reader_loop ()
#6  0x0000bff179695490 [PAC] in main ()
[Inferior 1 (process 2541) detached]
```
### gdb
```bash
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#define MSG "In function %20s; &localvar = %p\n"

static void bar_is_now_closed(void) {
    int localvar = 5;
    printf(MSG, __FUNCTION__, &localvar);
    printf("\n Now blocking on pause()...\n");

    pause();
}

static void bar(void) {
    int localvar = 5;
    printf(MSG, __FUNCTION__, &localvar);
    bar_is_now_closed();
}

static void foo(void) {
    int localvar = 5;
    printf(MSG, __FUNCTION__, &localvar);
    bar();
}

int main(int argc, char **argv) {
    int localvar = 5;
    printf(MSG, __FUNCTION__, &localvar);
    foo();
    exit(EXIT_SUCCESS);
}
```
Скомпілюємо, запустимо та отримаємо PID програми:

```bash
$ make 24
cc     24.c   -o 24
$ ./24 &
[2] 2325
$ In function                 main; &localvar = 0x7ff4e380ac
In function                  foo; &localvar = 0x7ff4e3807c
In function                  bar; &localvar = 0x7ff4e3805c
In function    bar_is_now_closed; &localvar = 0x7ff4e3803c

 Now blocking on pause()...

 $ pidof test
4554
```
Тепер під'єднаємося за допомогою `gdb`:

```bash
$ sudo gdb attach 4554
[sudo] password for andriib: 
GNU gdb (Ubuntu 15.0.50.20240403-0ubuntu1) 15.0.50.20240403-git
Copyright (C) 2024 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "aarch64-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<https://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
attach: No such file or directory.
Attaching to process 4554
Reading symbols from /home/andriib/test...
Reading symbols from /lib/aarch64-linux-gnu/libc.so.6...
Reading symbols from /usr/lib/debug/.build-id/d5/ef86dde36cbd3289566cf5098226035d76f2e1.debug...
Reading symbols from /lib/ld-linux-aarch64.so.1...
Reading symbols from /usr/lib/debug/.build-id/f3/d28c5cab7887a8195f6b130d76b8faf126b168.debug...
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/aarch64-linux-gnu/libthread_db.so.1".
0x0000f24699956b4c in __libc_pause () at ../sysdeps/unix/sysv/linux/pause.c:31

warning: 31	../sysdeps/unix/sysv/linux/pause.c: No such file or directory
(gdb)
```

Виконаємо команду `bt`:

```bash
(gdb) bt
#0  0x0000f24699956b4c in __libc_pause () at ../sysdeps/unix/sysv/linux/pause.c:31
#1  0x0000b23c666a096c in bar_is_now_closed () at test.c:13
#2  0x0000b23c666a09e4 in bar () at test.c:19
#3  0x0000b23c666a0a5c in foo () at test.c:25
#4  0x0000b23c666a0adc in main (argc=1, argv=0xfffffda4f1a8) at test.c:31

```

Завдання виконано

## Завдання 2.5

Завдання:

```
Відомо, що при виклику процедур і поверненні з них процесор
використовує стек.Чи можна в такій схемі обійтися без лічильника команд
(IP), використовуючи замість нього вершину стека? Обґрунтуйте свою
відповідь та наведіть приклади.
```

Ні, повністю відмовитися від лічильника команд IP/PC неможливо, оскільки він виконує унікальну функцію, яку стек не може замінити повноцінно.
Стек призначений для зберігання тимчасових даних та адрес повернення (щоб знати, куди повернутися після виконання функції). Лічильник команд (IP) потрібен для того, щоб процесор знав, яку інструкцію виконувати прямо зараз і де брати наступну.Більшість програм виконуються лінійно (команда за командою). Якби ми використовували тільки стек, нам довелося б постійно записувати на його вершину адресу кожної наступної інструкції вручну, що зробило б програму величезною та повільною.Процесор повинен мати фізичний регістр, який вказує на поточну точку виконання коду в пам'яті. Навіть якщо програміст не бачить цього регістра, процесор використовує його внутрішньо.

Існують так звані стекові архітектури (наприклад, віртуальна машина Java (JVM) або старі мейнфрейми Burroughs). У них усі обчислення відбуваються на стеку, і у команд часто немає явних операндів. Проте навіть ці системи мають внутрішній лічильник команд (PC), який відстежує, яку саме байт-код інструкцію треба завантажити та виконати наступною. Тобто стек допомагає керувати викликами функцій, але не замінює IP.
