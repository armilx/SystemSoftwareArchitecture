# Практична робота №8

## На тему: Системні виклики в UNIX/POSIX (файлові операції, fork(), qsort(), write(), read(), lseek() тощо)

## Завдання 8.1

**Умова:** Чи може виклик `count = write(fd, buffer, nbytes);` повернути в змінній `count` значення, відмінне від `nbytes`? Якщо так, то чому? Наведіть робочий приклад програми, яка демонструє вашу відповідь.

### Відповідь
Так, системний виклик `write` може повернути значення, менше за `nbytes` (це називається частковим записом). Головні причини:
1. Запис іде в системний канал (pipe) або сокет, і його внутрішній буфер заповнився.
2. Процес запису був перерваний системним сигналом.
3. На диску закінчилося вільне місце.
4. Перевищено ліміт розміру файлу для користувача.

### Опис рішення
Для демонстрації використовується системний канал (pipe). Його стандартний розмір у Linux — 65536 байт (64 КБ). Програма створює канал, переводить його в неблокуючий режим і намагається записати туди масив розміром 100 000 байт за один раз. Оскільки масив більший за буфер, `write` записує лише те, що помістилося, і повертає фактичну кількість записаних байтів.

### Код програми (task8_1.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(void) {
    int pipefd[2];
    char buffer[100000] = {0};

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    int flags = fcntl(pipefd[1], F_GETFL, 0);
    fcntl(pipefd[1], F_SETFL, flags | O_NONBLOCK);

    ssize_t count = write(pipefd[1], buffer, sizeof(buffer));

    printf("Tried to write: %lu bytes\n", (unsigned long)sizeof(buffer));
    printf("Actually wrote: %zd bytes\n", count);

    close(pipefd[0]);
    close(pipefd[1]);

    return EXIT_SUCCESS;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ gcc -Wall task8_1.c -o task8_1
andriib@andriib:~$ ./task8_1
Tried to write: 100000 bytes
Actually wrote: 65536 bytes
```

## Завдання 8.2

**Умова:** Є файл, дескриптор якого — `fd`. Файл містить таку послідовність байтів: 4, 5, 2, 2, 3, 3, 7, 9, 1, 5. У програмі виконується наступна послідовність системних викликів: `lseek(fd, 3, SEEK_SET); read(fd, &buffer, 4);` де виклик `lseek` переміщує покажчик на третій байт файлу. Що буде містити буфер після завершення виклику `read`? Наведіть робочий приклад програми, яка демонструє вашу відповідь.

### Відповідь
Буфер міститиме байти: **2, 3, 3, 7**. 
Індексація у файлі починається з нуля. Виклик `lseek(fd, 3, SEEK_SET)` переміщує покажчик на зміщення 3 (тобто на 4-й за рахунком байт). Четвертий байт у масиві — це `2`. Далі `read` зчитує 4 байти поспіль, починаючи з цієї позиції.

### Опис рішення
Програма створює тимчасовий файл і записує туди задану послідовність із 10 байтів (як масив `unsigned char`). Далі виконуються виклики `lseek` та `read` рівно за умовою завдання. Зчитані дані виводяться на екран у вигляді чисел, після чого тимчасовий файл видаляється за допомогою `unlink()`.

### Код програми (task8_2.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(void) {
    int fd = open("test_8_2.bin", O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    unsigned char data[] = {4, 5, 2, 2, 3, 3, 7, 9, 1, 5};
    write(fd, data, sizeof(data));

    lseek(fd, 3, SEEK_SET);

    unsigned char buffer[4] = {0};
    read(fd, buffer, 4);

    printf("Buffer contains: ");
    for (int i = 0; i < 4; i++) {
        printf("%d ", buffer[i]);
    }
    printf("\n");

    close(fd);
    unlink("test_8_2.bin");

    return EXIT_SUCCESS;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ gcc -Wall task8_2.c -o task8_2
andriib@andriib:~$ ./task8_2
Buffer contains: 2 3 3 7 
```

## Завдання 8.3

**Умова:** Напишіть програму, яка досліджує, які вхідні дані є найгіршими для алгоритму швидкого сортування (`qsort`). Автоматизуйте процес експериментування. Придумайте і реалізуйте набір тестів для перевірки правильності функції `qsort`.

### Опис рішення
Для аналізу ефективності `qsort` використовується глобальна змінна `cmp_count`, яка рахує кількість викликів функції порівняння. Програма спочатку тестує правильність `qsort` на одному базовому невідсортованому масиві, після чого генерує чотири великих масиви по 10 000 елементів (випадковий, відсортований, зворотно відсортований та з однаковими значеннями). Після кожного сортування програма порівнює кількість операцій і автоматично визначає, який набір даних виявився найгіршим для системної реалізації алгоритму.
### Код програми (task8_3.c)

```c
#include <stdio.h>
#include <stdlib.h>

long long cmp_count = 0;

int compare(const void *a, const void *b) {
    cmp_count++;
    int val_a = *(const int *)a;
    int val_b = *(const int *)b;
    return (val_a > val_b) - (val_a < val_b);
}

int main(void) {
    int test[] = {5, 2, 9, 1, 5, 6};
    qsort(test, 6, sizeof(int), compare);
    
    printf("Correctness test: ");
    for(int i = 0; i < 6; i++) printf("%d ", test[i]);
    printf("\n\n");

    int n = 10000;
    int *arr = malloc(n * sizeof(int));
    long long max_cmp = 0;
    const char *worst_case = "";

    for(int i = 0; i < n; i++) arr[i] = rand() % 10000;
    cmp_count = 0;
    qsort(arr, n, sizeof(int), compare);
    printf("Random: %lld ops\n", cmp_count);
    if (cmp_count > max_cmp) { max_cmp = cmp_count; worst_case = "Random"; }

    for(int i = 0; i < n; i++) arr[i] = i;
    cmp_count = 0;
    qsort(arr, n, sizeof(int), compare);
    printf("Sorted: %lld ops\n", cmp_count);
    if (cmp_count > max_cmp) { max_cmp = cmp_count; worst_case = "Sorted"; }

    for(int i = 0; i < n; i++) arr[i] = n - i;
    cmp_count = 0;
    qsort(arr, n, sizeof(int), compare);
    printf("Reverse: %lld ops\n", cmp_count);
    if (cmp_count > max_cmp) { max_cmp = cmp_count; worst_case = "Reverse"; }

    for(int i = 0; i < n; i++) arr[i] = 7;
    cmp_count = 0;
    qsort(arr, n, sizeof(int), compare);
    printf("Identical: %lld ops\n", cmp_count);
    if (cmp_count > max_cmp) { max_cmp = cmp_count; worst_case = "Identical"; }

    printf("\nWorst case is '%s' with %lld comparisons.\n", worst_case, max_cmp);

    free(arr);
    return 0;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ gcc -Wall task8_3.c -o task8_3
andriib@andriib:~$ ./task8_3
Correctness test: 1 2 5 5 6 9 

Random: 120520 ops
Sorted: 64608 ops
Reverse: 69008 ops
Identical: 64608 ops

Worst case is 'Random' with 120520 comparisons.

```

## Завдання 8.4

**Умова:** Виконайте наступну програму на мові програмування С:
```c
int main() {
  int pid;
  pid = fork();
  printf("%d\n", pid);
}
```
Завершіть цю програму. Припускаючи, що виклик `fork()` був успішним, яким може бути результат виконання цієї програми?

### Відповідь та опис рішення
Функція `fork()` створює точну копію поточного процесу (дочірній процес). Після її виклику код виконують вже **два** процеси одночасно:
1. У дочірньому процесі `fork()` повертає `0`.
2. У батьківському процесі `fork()` повертає PID (ідентифікатор) створеного дочірнього процесу (додатне число).

Оскільки `printf` стоїть після `fork()`, він виконається двічі (по одному разу в кожному процесі). Тому результатом роботи програми будуть **два числа**: нуль та PID дочірнього процесу. Порядок їх виведення може бути будь-яким, оскільки процеси виконуються паралельно.

Для коректної роботи програми потрібно додати необхідні бібліотеки (`<stdio.h>` та `<unistd.h>`) і додати `return 0;` у кінці.

### Код програми (task8_4.c)

```c
#include <stdio.h>
#include <unistd.h>

int main(void) {
    int pid;
    pid = fork();
    printf("%d\n", pid);
    return 0;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ gcc -Wall task8_4.c -o task8_4
andriib@andriib:~$ ./task8_4
3212
0
```
### Висновок
Під час виконання роботи успішно відпрацьовано взаємодію з базовими системними викликами UNIX/POSIX мовою C. Досліджено файлові операції (`read`, `write`, `lseek`) та розібрано системні причини часткового запису даних. На практиці вивчено механізм створення дочірніх процесів за допомогою функції `fork()` та специфіку їхнього паралельного виконання. Крім того, проведено експериментальне тестування алгоритму `qsort`, що дозволило проаналізувати його ефективність на різних наборах даних.
