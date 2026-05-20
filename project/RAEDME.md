## Мініпроєкт. Варіант 2

**Умова:** Менеджер фонових процесів. Створити програму, яка запускає кілька дочірніх процесів у фоновому режимі та дозволяє переглядати їхній стан. Програма має зберігати їхні PID, перевіряти, які з них завершилися, і коректно прибирати процеси-зомбі за допомогою `fork()`, `waitpid(..., WNOHANG)`, `kill()`, сигналів.

**Мета роботи:** закріпити навички управління процесами, сигналами та життєвим циклом дочірніх програм в ОС Linux шляхом створення менеджера фонових процесів.

**Використані системні виклики:**
* `fork()` — для створення нового дочірнього процесу.
* `execvp()` — для запуску переданої системної команди в дочірньому процесі.
* `waitpid(..., WNOHANG)` — для неблокуючої перевірки стану процесів та автоматичного прибирання неіснуючий процесів.
* `kill()` — для надсилання системного сигналу (`SIGTERM`) з метою зупинки процесу.

### Опис рішення
Програма реалізує простий інтерактивний інтерфейс. Команда `start` створює дочірній процес через `fork()` та виконує задану системну команду у фоновому режимі. Команда `list` виводить список збережених PID та назв активних процесів. Перед кожним запитом вводу викликається спеціальна функція, яка через `waitpid` з прапорцем `WNOHANG` без блокування перевіряє стан усіх збережених процесів: якщо процес завершився, він видаляється зі списку. Для примусової зупинки використовується команда `kill`, яка надсилає системний сигнал `SIGTERM`.

### Код програми (project_bg.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

typedef struct {
    pid_t pid;
    char name[64];
    int active;
} ProcessInfo;

ProcessInfo bg_procs[100];
int proc_count = 0;

void check() {
    for (int i = 0; i < proc_count; i++) {
        if (bg_procs[i].active) {
            int status;
            pid_t result = waitpid(bg_procs[i].pid, &status, WNOHANG);
            if (result > 0) {
                bg_procs[i].active = 0;
                printf("\n[Process PID %d finished]\n", bg_procs[i].pid);
            }
        }
    }
}

int main(void) {
    char input[256];
    
    while (1) {
        check();
        
        printf("bg_mgr> ");
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0) {
            break;
        } else if (strncmp(input, "start ", 6) == 0) {
            char *cmd = input + 6;
            pid_t pid = fork();
            
            if (pid == 0) {
                char *args[] = {"/bin/sh", "-c", cmd, NULL};
                execvp(args[0], args);
                exit(1);
            } else if (pid > 0) {
                bg_procs[proc_count].pid = pid;
                strncpy(bg_procs[proc_count].name, cmd, 63);
                bg_procs[proc_count].active = 1;
                proc_count++;
                printf("Started [%d] %s\n", pid, cmd);
            }
        } else if (strcmp(input, "list") == 0) {
            for (int i = 0; i < proc_count; i++) {
                if (bg_procs[i].active) {
                    printf("PID: %d | Cmd: %s\n", bg_procs[i].pid, bg_procs[i].name);
                }
            }
        } else if (strncmp(input, "kill ", 5) == 0) {
            int kpid = atoi(input + 5);
            if (kpid > 0) {
                kill(kpid, SIGTERM);
                printf("Sent SIGTERM to %d\n", kpid);
            }
        } else if (strlen(input) > 0) {
            printf("Commands: start <cmd>, list, kill <pid>, exit\n");
        }
    }
    
    return 0;
}
```

### Компіляція та запуск
```bash
andriib@andriib:~$ gcc -Wall project_bg.c -o project_bg
andriib@andriib:~$ ./project_bg
bg_mgr> start sleep 15
Started [5684] sleep 15
bg_mgr> start sleep 60
Started [5686] sleep 60
bg_mgr> list
PID: 5684 | Cmd: sleep 15
PID: 5686 | Cmd: sleep 60

[Process PID 5684 finished]
bg_mgr> kill 5686
Sent SIGTERM to 5686
bg_mgr> list
PID: 5686 | Cmd: sleep 60

[Process PID 5686 finished]
bg_mgr> exit

```
**Скріншоти роботи:**
<img width="980" height="900" alt="image" src="https://github.com/user-attachments/assets/9b882438-6509-459d-be72-6d03aa921877" />

### Висновок
У ході виконання мініпроєкту закріплено базові навички системного програмування у Linux. Успішно розроблено консольний менеджер фонових процесів, який демонструє керування життєвим циклом дочірніх програм. На практиці відпрацьовано асинхронне відстеження стану процесів за допомогою системного виклику `waitpid` з прапорцем `WNOHANG`, передачу системних сигналів через `kill`.
