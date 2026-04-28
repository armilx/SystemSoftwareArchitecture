# Варіант 2

**Умова:** Реалізуйте утиліту командного рядка, яка виводить процеси, запущені лише з нестандартних шеллів, не використовуючи `ps`, `top`, `htop`.

### Опис рішення
Програма аналізує вміст директорії `/proc`, де кожна папка з числовим іменем відповідає активному процесу. 
1. Програма відкриває `/proc` за допомогою `opendir()`.
2. Для кожного PID (ідентифікатора процесу) зчитується символічне посилання `/proc/[PID]/exe` за допомогою `readlink()`. Це дозволяє отримати повний шлях до виконуваного файлу процесу.
3. Отриманий шлях порівнюється зі списком "стандартних" шеллів (наприклад, `/bin/bash`, `/bin/sh`, `/bin/dash`). 
4. Якщо процес є шеллом (його назва закінчується на `sh`), але він не входить до стандартного списку (наприклад, `zsh`, `fish` або саморобний шелл) — програма виводить його PID та шлях.

### Код програми (p7var2.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>

int is_pid_dir(const char *name) {
    for (int i = 0; name[i] != '\0'; i++) {
        if (!isdigit(name[i])) return 0;
    }
    return 1;
}

int main(void) {
    DIR *dir = opendir("/proc");
    struct dirent *entry;
    char path[512], exe_path[512];

    if (!dir) {
        perror("opendir /proc failed");
        return EXIT_FAILURE;
    }

    printf("%-10s %s\n", "PID", "NON-STANDARD SHELL PATH");
    printf("------------------------------------------\n");

    while ((entry = readdir(dir)) != NULL) {
        if (!is_pid_dir(entry->d_name)) continue;

        snprintf(path, sizeof(path), "/proc/%s/exe", entry->d_name);
        ssize_t len = readlink(path, exe_path, sizeof(exe_path) - 1);

        if (len != -1) {
            exe_path[len] = '\0';

            if (strstr(exe_path, "sh") != NULL) {
                if (strcmp(exe_path, "/bin/bash") != 0 &&
                    strcmp(exe_path, "/bin/sh") != 0 &&
                    strcmp(exe_path, "/usr/bin/bash") != 0 &&
                    strcmp(exe_path, "/bin/dash") != 0) {
                    printf("%-10s %s\n", entry->d_name, exe_path);
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
andriib@andriib:~$ gcc -Wall p7var2.c -o p7var2
andriib@andriib:~$ ./p7var2
PID        NON-STANDARD SHELL PATH
------------------------------------------
2276       /usr/libexec/gcr-ssh-agent
2346       /usr/bin/gnome-shell
2406       /usr/libexec/gnome-shell-calendar-server
2469       /usr/libexec/gsd-sharing
2703       /usr/libexec/gvfsd-trash (deleted)

```

### Висновок
Програма дозволяє виявити процеси, що використовують альтернативні командні оболонки, шляхом прямого звернення до інтерфейсу ядра `/proc`. Це забезпечує незалежність від високорівневих утиліт моніторингу та дозволяє аналізувати дерево процесів на рівні системних викликів.
