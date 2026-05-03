## Варіант 2

**Умова:** Реалізуйте інтерпретатор команд, який підтримує черги читання-запису між процесами через pipe, не використовуючи shell.

### Опис рішення
Програма працює у циклі, приймаючи ввід від користувача. При виявленні символу `|` рядок розділяється на дві частини (команди). Створюється канал `pipe()`, після чого через `fork()` запускаються два дочірніх процеси. Перший процес перенаправляє свій вивід у канал через `dup2()` та виконує першу команду. Другий процес перенаправляє свій ввід з каналу та виконує другу команду. Використовується `execvp`, тому системна оболонка не залучається. Батьківський процес очікує завершення обох дочірніх процесів.

### Код програми (var2task8.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void execute_pipeline(char *cmd1_str, char *cmd2_str) {
    char *argv1[32], *argv2[32];
    int i = 0;

    char *token = strtok(cmd1_str, " \n");
    while (token) { argv1[i++] = token; token = strtok(NULL, " \n"); }
    argv1[i] = NULL;

    i = 0;
    token = strtok(cmd2_str, " \n");
    while (token) { argv2[i++] = token; token = strtok(NULL, " \n"); }
    argv2[i] = NULL;

    if (!argv1[0] || !argv2[0]) return;

    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        return;
    }

    if (fork() == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execvp(argv1[0], argv1);
        perror("execvp cmd1 failed");
        exit(1);
    }

    if (fork() == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        execvp(argv2[0], argv2);
        perror("execvp cmd2 failed");
        exit(1);
    }

    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);
}

int main(void) {
    char input[256];
    
    printf("Type 'exit' to quit.\n");
    
    while (1) {
        printf("myshell> ");
        if (!fgets(input, sizeof(input), stdin)) break;
        
        if (strncmp(input, "exit", 4) == 0) break;

        char *pipe_pos = strchr(input, '|');
        if (pipe_pos) {
            *pipe_pos = '\0';
            execute_pipeline(input, pipe_pos + 1);
        } else {
            printf("Error: This shell only supports piped commands (e.g., ls -l | grep txt)\n");
        }
    }
    
    return 0;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ gcc -Wall var2task8.c -o var2task8
andriib@andriib:~$ ./var2task8
Type 'exit' to quit.
myshell> ls -l | grep task8
-rwxrwxr-x 1 andriib andriib 70800 Apr 28 19:11 task8
-rwxrwxr-x 1 andriib andriib 70672 May  3 06:35 task8_1
-rw-rw-r-- 1 andriib andriib   595 May  3 06:35 task8_1.c
-rwxrwxr-x 1 andriib andriib 70752 May  3 06:36 task8_2
-rw-rw-r-- 1 andriib andriib   627 May  3 06:36 task8_2.c
-rwxrwxr-x 1 andriib andriib 70720 May  3 06:41 task8_3
-rw-rw-r-- 1 andriib andriib  1616 May  3 06:44 task8_3.c
-rwxrwxr-x 1 andriib andriib 70360 May  3 06:44 task8_4
-rw-rw-r-- 1 andriib andriib   129 May  3 06:44 task8_4.c
-rw-rw-r-- 1 andriib andriib   958 Apr 28 19:10 task8.c
-rwxrwxr-x 1 andriib andriib 71040 May  3 07:43 var2task8
-rw-rw-r-- 1 andriib andriib  1628 May  3 07:43 var2task8.c
myshell> exit

```

### Висновок
Під час виконання практичної роботи закріплено навички роботи з системними викликами UNIX/POSIX. Досліджено механізми переміщення покажчика у файлі (`lseek`), читання та запису даних (`read`, `write`), а також випадки часткового запису. На практиці реалізовано створення процесів через `fork()`, перенаправлення потоків вводу-виводу за допомогою `dup2()` та організацію міжпроцесної взаємодії через канали (`pipe`). Крім того, проведено аналіз ефективності системної функції `qsort` на різних наборах даних. Отримані знання дозволяють працювати з процесами та файловою системою на низькому рівні.
