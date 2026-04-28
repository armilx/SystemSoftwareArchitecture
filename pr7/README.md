# Практична робота №7 

## На тему: Дослідження, моделювання та нестандартні підходи до аналізу процесів, файлових систем, безпеки та ресурсів в Linux

## Задача 1

**Умова:** Використайте `popen()`, щоб передати вивід команди `rwho` (команда UNIX) до `more` (команда UNIX) у програмі на C.

### Опис рішення
Програма використовує функцію `popen()` для створення каналів (pipes). Спочатку викликається `popen("rwho", "r")`, щоб запустити команду `rwho` та прочитати її вивід. Потім відкривається другий потік `popen("more", "w")` для передачі отриманих даних на вхід утиліті `more`. Далі дані читаються у циклі з першого потоку і записуються в другий.

### Код програми (task1.c)

```c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FILE *cmd_in, *cmd_more;
    char buffer[1024];

    cmd_in = popen("rwho", "r");
    if (cmd_in == NULL) {
        perror("popen rwho failed");
        exit(EXIT_FAILURE);
    }

    cmd_more = popen("more", "w");
    if (cmd_more == NULL) {
        perror("popen more failed");
        pclose(cmd_in);
        exit(EXIT_FAILURE);
    }

    while (fgets(buffer, sizeof(buffer), cmd_in) != NULL) {
        fputs(buffer, cmd_more);
    }

    pclose(cmd_in);
    pclose(cmd_more);

    return EXIT_SUCCESS;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ gcc -Wall task1.c -o task1
andriib@andriib:~$ ./task1
/var/spool/rwho: Permission denied
```
## Задача 2

**Умова:** Напишіть програму мовою C, яка імітує команду `ls -l` в UNIX — виводить список усіх файлів у поточному каталозі та перелічує права доступу тощо. (Варіант вирішення, що просто виконує `ls -l` із вашої програми, — не підходить.)

### Опис рішення
Для реалізації власного `ls -l` програма використовує системні виклики та бібліотеки Linux:
1. `opendir()` та `readdir()` — для читання вмісту поточної директорії (`.`).
2. `stat()` — для отримання метаданих кожного файлу (розмір, час модифікації, права).
3. Права доступу розшифровуються за допомогою побітового "І" (`&`) та системних масок.
4. `getpwuid()` та `getgrgid()` — конвертують числові ідентифікатори (UID/GID) у текстові імена користувача та групи.
5. Для сумісності з прапорцем `-Wall` додано явне приведення типів при виведенні даних.
6. Програма ігнорує приховані файли (ті, що починаються з крапки).

### Код програми (task2.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void print_permissions(mode_t mode) {
    putchar(S_ISDIR(mode) ? 'd' : '-');
    putchar((mode & S_IRUSR) ? 'r' : '-');
    putchar((mode & S_IWUSR) ? 'w' : '-');
    putchar((mode & S_IXUSR) ? 'x' : '-');
    putchar((mode & S_IRGRP) ? 'r' : '-');
    putchar((mode & S_IWGRP) ? 'w' : '-');
    putchar((mode & S_IXGRP) ? 'x' : '-');
    putchar((mode & S_IROTH) ? 'r' : '-');
    putchar((mode & S_IWOTH) ? 'w' : '-');
    putchar((mode & S_IXOTH) ? 'x' : '-');
    putchar(' ');
}

int main(void) {
    DIR *dir = opendir(".");
    if (!dir) return 1;

    struct dirent *entry;
    struct stat fs;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        if (stat(entry->d_name, &fs) == -1) continue;

        print_permissions(fs.st_mode);
        
        printf("%2lu ", (unsigned long)fs.st_nlink);

        struct passwd *pw = getpwuid(fs.st_uid);
        struct group *gr = getgrgid(fs.st_gid);
        printf("%s %s ", pw ? pw->pw_name : "-", gr ? gr->gr_name : "-");

        printf("%8ld ", (long)fs.st_size);

        char timebuf[80];
        struct tm *t = localtime(&fs.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", t);
        printf("%s ", timebuf);

        printf("%s\n", entry->d_name);
    }

    closedir(dir);
    return 0;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ gcc -Wall task2.c -o task2
andriib@andriib:~$ ./task2
-rw-rw-r--  1 andriib andriib      323 Apr 14 11:49 new_realloc.c
-rwxrwxr-x  1 andriib andriib    72760 Feb 18 14:13 test
-rw-rw-r--  1 andriib andriib      861 Feb 17 11:41 time.c
-rw-rw-r--  1 andriib andriib      560 Apr 14 10:55 malloc_test.c
-rw-rw-r--  1 andriib andriib      435 Apr 14 11:06 zero_malloc.c
-rw-rw-r--  1 andriib andriib      116 Feb 17 12:07 helloworld.c
-rwxrwxr-x  1 andriib andriib    70440 Apr 28 15:47 overflow
drwxr-xr-x  2 andriib andriib     4096 Feb 10 12:48 Desktop
-rwxrwxr-x  1 andriib andriib    70448 Apr 28 15:21 heap_corruption
-rwxrwxr-x  1 andriib andriib    70448 Apr 14 10:57 malloc_test
drwxrwxr-x  2 andriib andriib     4096 Feb 18 14:48 pr2
drwxr-xr-x  2 andriib andriib     4096 Feb 10 12:48 Music
-rwxrwxr-x  1 andriib andriib    70440 Apr 14 11:20 good_ptr
-rw-rw-r--  1 andriib andriib     1137 Apr 14 13:13 leak.c
drwxrwxr-x  2 andriib andriib     4096 Feb 10 12:52 projects
-rwxrwxr-x  1 andriib andriib    70488 Apr 14 11:26 realloc_test
-rwxrwxr-x  1 andriib andriib      267 Feb 06 21:30 scan_libs.sh
-rw-rw-r--  1 andriib andriib      556 Feb 17 12:18 memory_layout.c
-rwxrwxr-x  1 andriib andriib    70576 Feb 17 12:19 memory_test
drwxr-xr-x  2 andriib andriib     4096 Feb 10 12:48 Downloads
drwx------  3 andriib andriib     4096 Feb 10 12:54 snap
-rwxrwxr-x  1 andriib andriib    70488 Apr 14 11:39 realloc_edge
drwxr-xr-x  2 andriib andriib     4096 Feb 10 12:48 Public
-rw-rw-r--  1 andriib andriib      420 Apr 14 11:20 good_ptr.c
-rw-rw-r--  1 andriib andriib      652 Feb 18 14:13 test.c
-rw-rw-r--  1 andriib andriib      863 Apr 14 11:25 realloc_test.c
-rwxrwxr-x  1 andriib andriib    70464 Feb 17 12:07 helloworld
-rwxrwxr-x  1 andriib andriib    70384 Apr 14 11:49 old_realloc
-rw-rw-r--  1 andriib andriib     1462 Apr 28 17:02 task2.c
-rwxrwxr-x  1 andriib andriib    70440 Apr 14 11:15 bad_ptr
-rw-------  1 andriib andriib      207 Apr 28 16:13 corruption.c.save
-rwxrwxr-x  1 andriib andriib    70448 Apr 14 11:06 zero_malloc
-rw-rw-r--  1 andriib andriib      206 Apr 28 16:13 corruption.c
-rw-rw-r--  1 andriib andriib      319 Apr 14 11:48 old_realloc.c
-rw-rw-r--  1 andriib andriib      545 Apr 28 15:47 overflow.c
-rwxrwxr-x  1 andriib andriib    70624 Apr 28 16:43 task1
-rw-rw-r--  1 andriib andriib      501 Apr 14 11:15 bad_ptr.c
-rw-rw-r--  1 andriib andriib      570 Apr 28 16:43 task1.c
drwxr-xr-x  2 andriib andriib     4096 Feb 10 12:48 Pictures
-rwxrwxr-x  1 andriib andriib    70584 Feb 17 11:41 time_test
drwxr-xr-x  2 andriib andriib     4096 Feb 10 12:48 Templates
drwxr-xr-x  2 andriib andriib     4096 Feb 10 12:48 Documents
-rwxrwxr-x  1 andriib andriib    80392 Apr 28 16:20 corr_asan
-rwxrwxr-x  1 andriib andriib    70840 Apr 14 13:14 leak
-rwxrwxr-x  1 andriib andriib    70384 Apr 14 11:49 new_realloc
-rw-rw-r--  1 andriib andriib     1478 Apr 28 15:21 heap_corruption.c
-rwxrwxr-x  1 andriib andriib    70896 Apr 28 17:02 task2
-rwxrwxr-x  1 andriib andriib    70336 Apr 28 16:14 corr
drwxr-xr-x  2 andriib andriib     4096 Feb 10 12:48 Videos
-rw-rw-r--  1 andriib andriib      548 Apr 14 11:39 realloc_edge.c

```
## Задача 3

**Умова:** Напишіть програму, яка друкує рядки з файлу, що містять слово, передане як аргумент програми (проста версія утиліти `grep` в UNIX).

### Опис рішення
Програма приймає два аргументи командного рядка: слово для пошуку та ім'я файлу. Вона відкриває вказаний файл для читання і за допомогою функції `fgets()` зчитує його вміст рядок за рядком. Для перевірки наявності потрібного слова у ліченому рядку використовується стандартна функція `strstr()` з бібліотеки `<string.h>`. Якщо підрядок знайдено, функція повертає вказівник (не `NULL`), і весь рядок виводиться у стандартний потік виводу. 

### Код програми (task3.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <word> <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[2], "r");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    char buffer[2048];
    while (fgets(buffer, sizeof(buffer), file)) {
        if (strstr(buffer, argv[1])) {
            printf("%s", buffer);
        }
    }

    fclose(file);
    return EXIT_SUCCESS;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ gcc -Wall task3.c -o task3
andriib@andriib:~$ echo -e "hello world\nthis is a test\nwe love the world\nlinux is great" > test.txt
andriib@andriib:~$ ./task3 world test.txt
hello world
we love the world
```
## Задача 4

**Умова:** Напишіть програму, яка виводить список файлів, заданих у вигляді аргументів, з зупинкою кожні 20 рядків, доки не буде натиснута клавіша (спрощена версія утиліти `more` в UNIX).

### Опис рішення
Програма ітерується по всіх переданих аргументах командного рядка (іменах файлів). Кожен файл відкривається для читання, і його вміст виводиться рядок за рядком за допомогою `fgets()`. Введено лічильник рядків: як тільки він досягає 20, програма призупиняє виконання і чекає на введення від користувача. Для коректного зчитування натискання клавіші (навіть якщо стандартний ввід був би перенаправлений) програма читає символ безпосередньо з терміналу (`/dev/tty`). Після натискання клавіші (Enter) лічильник скидається, і виведення продовжується.

### Код програми (task4.c)

```c
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1> [file2...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    char buffer[2048];
    int line_count = 0;
    
    FILE *tty = fopen("/dev/tty", "r");
    if (!tty) {
        tty = stdin;
    }

    for (int i = 1; i < argc; i++) {
        FILE *file = fopen(argv[i], "r");
        if (!file) {
            perror(argv[i]);
            continue;
        }

        while (fgets(buffer, sizeof(buffer), file)) {
            printf("%s", buffer);
            line_count++;

            if (line_count == 20) {
                fgetc(tty);
                line_count = 0;
            }
        }
        fclose(file);
    }

    if (tty != stdin) {
        fclose(tty);
    }

    return EXIT_SUCCESS;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ gcc -Wall task4.c -o task4
andriib@andriib:~$ ./task4 /etc/passwd
root:x:0:0:root:/root:/bin/bash
daemon:x:1:1:daemon:/usr/sbin:/usr/sbin/nologin
bin:x:2:2:bin:/bin:/usr/sbin/nologin
sys:x:3:3:sys:/dev:/usr/sbin/nologin
sync:x:4:65534:sync:/bin:/bin/sync
games:x:5:60:games:/usr/games:/usr/sbin/nologin
man:x:6:12:man:/var/cache/man:/usr/sbin/nologin
lp:x:7:7:lp:/var/spool/lpd:/usr/sbin/nologin
mail:x:8:8:mail:/var/mail:/usr/sbin/nologin
news:x:9:9:news:/var/spool/news:/usr/sbin/nologin
uucp:x:10:10:uucp:/var/spool/uucp:/usr/sbin/nologin
proxy:x:13:13:proxy:/bin:/usr/sbin/nologin
www-data:x:33:33:www-data:/var/www:/usr/sbin/nologin
backup:x:34:34:backup:/var/backups:/usr/sbin/nologin
list:x:38:38:Mailing List Manager:/var/list:/usr/sbin/nologin
irc:x:39:39:ircd:/run/ircd:/usr/sbin/nologin
_apt:x:42:65534::/nonexistent:/usr/sbin/nologin
nobody:x:65534:65534:nobody:/nonexistent:/usr/sbin/nologin
systemd-network:x:998:998:systemd Network Management:/:/usr/sbin/nologin
systemd-timesync:x:997:997:systemd Time Synchronization:/:/usr/sbin/nologin
```
## Задача 5

**Умова:** Напишіть програму, яка перелічує всі файли в поточному каталозі та всі файли в підкаталогах.

### Опис рішення
Програма використовує рекурсивний підхід для обходу дерева каталогів. Створено окрему функцію, яка приймає шлях до каталогу як аргумент. Вона відкриває його за допомогою `opendir()` і читає вміст через `readdir()`. Щоб уникнути нескінченного циклу, програма ігнорує поточний (`.`) та батьківський (`..`) каталоги. Для кожного знайденого елемента виводиться його повний шлях, після чого за допомогою `stat()` перевіряється, чи є цей елемент директорією. Якщо так — функція викликає саму себе, передаючи новий шлях, таким чином заглиблюючись у всі підкаталоги.

### Код програми (task5.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

void list_files_recursive(const char *base_path) {
    char path[2048];
    struct dirent *dp;
    DIR *dir = opendir(base_path);

    if (!dir) return;

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            snprintf(path, sizeof(path), "%s/%s", base_path, dp->d_name);
            printf("%s\n", path);

            struct stat statbuf;
            if (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
                list_files_recursive(path);
            }
        }
    }
    closedir(dir);
}

int main(void) {
    list_files_recursive(".");
    return EXIT_SUCCESS;
}
```

### Компіляція та запуск
```bash
andriib@andriib:~$ gcc -Wall task5.c -o task5
andriib@andriib:~$ ./task5 | head -n 15
./.sudo_as_admin_successful
./new_realloc.c
./test
./time.c
./malloc_test.c
./zero_malloc.c
./helloworld.c
./overflow
./Desktop
./heap_corruption
./malloc_test
./pr2
./pr2/tnmap
./pr2/malloc_t.c
./pr2/t_malloc
```
[вивід скорочено для компактності]

## Задача 6

**Умова:** Напишіть програму, яка перелічує лише підкаталоги у алфавітному порядку.

### Опис рішення
Для виконання цього завдання найефективніше використати системну функцію `scandir()`. Вона автоматично читає вміст директорії, фільтрує його за допомогою заданої користувачем функції та сортує результат. 
У програмі створено функцію фільтрації, яка відкидає стандартні посилання `.` та `..`, а для решти файлів за допомогою `stat()` перевіряє, чи є вони каталогами (`S_ISDIR`). Для сортування масиву результатів за алфавітом використовується вбудована функція `alphasort`, яка передається як аргумент у `scandir()`.

### Код програми (task6.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int filter_dirs(const struct dirent *entry) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        return 0;
    }
    struct stat statbuf;
    if (stat(entry->d_name, &statbuf) == 0) {
        return S_ISDIR(statbuf.st_mode);
    }
    return 0;
}

int main(void) {
    struct dirent **namelist;
    int n;

    n = scandir(".", &namelist, filter_dirs, alphasort);
    if (n < 0) {
        perror("scandir");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < n; i++) {
        printf("%s\n", namelist[i]->d_name);
        free(namelist[i]);
    }
    free(namelist);

    return EXIT_SUCCESS;
}
```

### Компіляція та запуск
```bash
andriib@andriib:~$ gcc -Wall task6.c -o task6
andriib@andriib:~$ ./task6
.cache
.config
.gnupg
.local
.ssh
Desktop
Documents
Downloads
Music
Pictures
Public
Templates
Videos
pr2
projects
snap
```
## Задача 7

**Умова:** Напишіть програму, яка показує користувачу всі його/її вихідні програми на C, а потім в інтерактивному режимі запитує, чи потрібно надати іншим дозвіл на читання (read permission); у разі ствердної відповіді — такий дозвіл повинен бути наданий.

### Опис рішення
Програма виконує завдання у два проходи по поточній директорії. Спочатку за допомогою `opendir()` та `readdir()` вона знаходить усі файли, що закінчуються на `.c`, і просто виводить їхній список на екран. Далі використовується функція `rewinddir()`, яка повертає вказівник читання директорії на самий початок. 
Під час другого проходу програма знову знаходить ті самі `.c` файли, але тепер для кожного запитує користувача (через `fgets()`), чи хоче він змінити права доступу. Якщо введено `y` або `Y`, програма зчитує поточні права файлу через `stat()` і додає до них право читання для інших користувачів (`S_IROTH`) за допомогою системного виклику `chmod()`.

### Код програми (task7.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int is_c_file(const char *name) {
    size_t len = strlen(name);
    return (len > 2 && name[len - 2] == '.' && name[len - 1] == 'c');
}

int main(void) {
    DIR *dir = opendir(".");
    struct dirent *dp;

    if (!dir) return EXIT_FAILURE;

    printf("C source files in current directory:\n");
    while ((dp = readdir(dir)) != NULL) {
        if (is_c_file(dp->d_name)) {
            printf("- %s\n", dp->d_name);
        }
    }

    rewinddir(dir);
    printf("\n");

    char buf[16];
    while ((dp = readdir(dir)) != NULL) {
        if (is_c_file(dp->d_name)) {
            printf("Grant read permission to others for %s? (y/n): ", dp->d_name);
            if (fgets(buf, sizeof(buf), stdin)) {
                if (buf[0] == 'y' || buf[0] == 'Y') {
                    struct stat st;
                    if (stat(dp->d_name, &st) == 0) {
                        chmod(dp->d_name, st.st_mode | S_IROTH);
                    }
                }
            }
        }
    }

    closedir(dir);
    return EXIT_SUCCESS;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ gcc -Wall task7.c -o task7
andriib@andriib:~$ ./task7
C source files in current directory:
- new_realloc.c
- time.c
- malloc_test.c
...
[список скорочено]

Grant read permission to others for new_realloc.c? (y/n): n
Grant read permission to others for time.c? (y/n): y
Grant read permission to others for malloc_test.c? (y/n): n
```

## Задача 8

**Умова:** Напишіть програму, яка надає користувачу можливість видалити будь-який або всі файли у поточному робочому каталозі. Має з’являтися ім’я файлу з запитом, чи слід його видалити.

### Опис рішення
Програма використовує `opendir()` та `readdir()` для ітерації по всіх елементах поточної директорії. За допомогою `stat()` та маски `S_ISREG` програма відфільтровує лише звичайні файли (ігноруючи самі директорії, щоб випадково не спробувати видалити папки `.` або `..`). Для кожного знайденого файлу користувачу виводиться запит через `printf()`. Якщо користувач вводить `y` або `Y` (зчитується через `fgets()`), програма викликає системну функцію `unlink()` для безповоротного видалення файлу з файлової системи.

### Код програми (task8.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    DIR *dir = opendir(".");
    struct dirent *dp;
    char buf[16];
    struct stat st;

    if (!dir) return EXIT_FAILURE;

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }

        if (stat(dp->d_name, &st) == 0 && S_ISREG(st.st_mode)) {
            printf("Delete file %s? (y/n): ", dp->d_name);
            if (fgets(buf, sizeof(buf), stdin)) {
                if (buf[0] == 'y' || buf[0] == 'Y') {
                    if (unlink(dp->d_name) == 0) {
                        printf("Deleted %s\n", dp->d_name);
                    } else {
                        perror("Error deleting file");
                    }
                }
            }
        }
    }

    closedir(dir);
    return EXIT_SUCCESS;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ gcc -Wall task8.c -o task8
andriib@andriib:~$ touch test_delete_me.txt
andriib@andriib:~$ ./task8
Delete file .sudo_as_admin_successful? (y/n): n
Delete file task8? (y/n): n
Delete file test_delete_me.txt? (y/n): y
Deleted test_delete_me.txt

```
## Задача 9

**Умова:** Напишіть програму на C, яка вимірює час виконання фрагмента коду в мілісекундах.

### Опис рішення
Для максимально точного вимірювання часу в мілісекундах у POSIX-системах використовується функція `clock_gettime()` із параметром `CLOCK_MONOTONIC`. На відміну від звичайного системного часу, монотонний годинник не залежить від переведення часу в системі чи синхронізації через інтернет, що робить його ідеальним для вимірювання інтервалів.
Програма фіксує час до початку виконання "важкого" циклу (який просто додає числа для імітації навантаження) та одразу після його завершення. Далі вираховується різниця в секундах та наносекундах, яка переводиться у мілісекунди для зручного виведення.

### Код програми (task9.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    struct timespec start, end;
    long long sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (long long i = 0; i < 100000000; i++) {
        sum += i;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double time_taken = (end.tv_sec - start.tv_sec) * 1000.0;
    time_taken += (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("Execution time: %.2f ms\n", time_taken);

    return EXIT_SUCCESS;
}
```

### Компіляція та запуск
```bash
andriib@andriib:~$ gcc -Wall task9.c -o task9
andriib@andriib:~$ ./task9
Execution time: 375.85 ms

```
## Задача 10

**Умова:** Напишіть програму мовою C для створення послідовності випадкових чисел з плаваючою комою у діапазонах: (a) від 0.0 до 1.0; (b) від 0.0 до n, де n — будь-яке дійсне число з плаваючою точкою. Початкове значення генератора випадкових чисел має бути встановлене так, щоб гарантувати унікальну послідовність.

### Опис рішення
Для генерації випадкових чисел використовується стандартна функція `rand()`. Щоб кожна послідовність була унікальною при кожному новому запуску програми, генератор ініціалізується функцією `srand(time(NULL))`, яка використовує поточний системний час як "зерно" (seed). 
Для отримання числа в діапазоні [0.0; 1.0], результат `rand()` ділиться на константу `RAND_MAX` (максимальне можливе значення, яке може видати генератор). Для отримання числа в діапазоні [0.0; n], отримане значення від 0 до 1 просто множиться на число `n`.

### Код програми (task10.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    double n = 25.5;
    srand(time(NULL));

    printf("Random numbers from 0.0 to 1.0:\n");
    for (int i = 0; i < 5; i++) {
        double r = (double)rand() / RAND_MAX;
        printf("%f\n", r);
    }

    printf("\nRandom numbers from 0.0 to %.2f:\n", n);
    for (int i = 0; i < 5; i++) {
        double r = ((double)rand() / RAND_MAX) * n;
        printf("%f\n", r);
    }

    return EXIT_SUCCESS;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ gcc -Wall task10.c -o task10
andriib@andriib:~$ ./task10
Random numbers from 0.0 to 1.0:
0.610887
0.173969
0.123448
0.415715
0.294621

Random numbers from 0.0 to 25.50:
25.323443
5.458192
12.078308
1.415854
15.094902

```
Висновок: під час роботи опановано системне програмування в Linux на C. Реалізовано механізми взаємодії процесів через канали, роботу з метаданими файлової системи та рекурсивний обхід каталогів. На прикладі аналогів утиліт ls, grep та more вивчено системні виклики stat, opendir та chmod. Також освоєно точне вимірювання часу виконання коду та генерацію випадкових чисел. Отримані навички дозволяють ефективно використовувати низькорівневі API для керування ресурсами ОС.
