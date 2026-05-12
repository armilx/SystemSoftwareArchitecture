## Варіант 2 (Практична робота 11)

**Умова:** Створіть демон, який відстежує процеси, що аварійно завершуються (через SIGFPE, SIGILL, SIGSEGV) та зберігає всю інформацію про контекст у базу даних SQLite.

### Опис рішення
Програма складається з двох частин: демона та клієнта. Демон непомітно працює у фоні, слухає локальний Unix-сокет і зберігає всі отримані дані в базу SQLite. Клієнт (тестова програма) має спеціальний обробник аварій. Коли клієнт падає (наприклад, через помилку пам'яті SIGSEGV), він перехоплює цей краш, швидко збирає свій PID та регістри, відправляє їх демону через сокет і лише після цього завершується.

### Код програми-демона (task11_daemon.c)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sqlite3.h>

#define SOCK_PATH "/tmp/crash.sock"
#define DB_PATH "/tmp/crashes.db"

typedef struct {
    int pid;
    int signo;
    unsigned long long fault_addr;
    unsigned long long rip;
    unsigned long long rsp;
} crash_ctx_t;

int main(void) {
    if (fork() != 0) exit(0);
    setsid();
    if (fork() != 0) exit(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    sqlite3 *db;
    if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) exit(1);

    const char *sql_create = "CREATE TABLE IF NOT EXISTS crashes (id INTEGER PRIMARY KEY, pid INT, signo INT, fault_addr TEXT, rip TEXT, rsp TEXT);";
    sqlite3_exec(db, sql_create, NULL, NULL, NULL);

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    unlink(SOCK_PATH);
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);
    bind(fd, (struct sockaddr*)&addr, sizeof(addr));

    crash_ctx_t ctx;
    while (1) {
        if (recv(fd, &ctx, sizeof(ctx), 0) > 0) {
            char sql[512];
            snprintf(sql, sizeof(sql),
                     "INSERT INTO crashes (pid, signo, fault_addr, rip, rsp) VALUES (%d, %d, '0x%llx', '0x%llx', '0x%llx');",
                     ctx.pid, ctx.signo, ctx.fault_addr, ctx.rip, ctx.rsp);
            sqlite3_exec(db, sql, NULL, NULL, NULL);
        }
    }
    
    return 0;
}
```

### Код програми-клієнта для тестування (task11_test.c)

```c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ucontext.h>
#include <ucontext.h>

#define SOCK_PATH "/tmp/crash.sock"

typedef struct {
    int pid;
    int signo;
    unsigned long long fault_addr;
    unsigned long long rip;
    unsigned long long rsp;
} crash_ctx_t;

static void crash_handler(int sig, siginfo_t *si, void *ctx) {
    crash_ctx_t msg;
    msg.pid = getpid();
    msg.signo = sig;
    msg.fault_addr = (unsigned long long)si->si_addr;

    (void)ctx;
#if defined(__x86_64__)
    ucontext_t *uc = (ucontext_t *)ctx;
    msg.rip = (unsigned long long)uc->uc_mcontext.gregs[REG_RIP];
    msg.rsp = (unsigned long long)uc->uc_mcontext.gregs[REG_RSP];
#else
    msg.rip = 0;
    msg.rsp = 0;
#endif

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    sendto(fd, &msg, sizeof(msg), 0, (struct sockaddr*)&addr, sizeof(addr));
    close(fd);

    _exit(1);
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
    
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);

    printf("Crashing process PID: %d\n", getpid());
    
    volatile int *p = NULL;
    *p = 42; 

    return 0;
}
```

### Компіляція та запуск

```bash
andriib@andriib:~$ gcc -Wall task11_daemon.c -o task11_daemon -lsqlite3
andriib@andriib:~$ gcc -Wall task11_test.c -o task11_test
andriib@andriib:~$ rm -f /tmp/crash.sock /tmp/crashes.db
andriib@andriib:~$ ./task11_daemon
andriib@andriib:~$ ./task11_test
Crashing process PID: 3436
andriib@andriib:~$ sqlite3 /tmp/crashes.db -header -column "SELECT * FROM crashes;"
id  pid   signo  fault_addr  rip  rsp
--  ----  -----  ----------  ---  ---
1   3436  11     0x0         0x0  0x0
```
*(Примітка: В нулі скинуті значення `rip` та `rsp`, оскільки тестування проводилось на архітектурі ARM, для якої зчитування контексту регістрів не імплементоване в коді програми).*

### Висновок
У ході роботи успішно створено фоновий процес (демон), який працює ізольовано від термінала. Налаштовано швидку передачу даних між процесами за допомогою локальних Unix-сокетів. На практиці реалізовано механізм перехоплення критичних помилок (SIGSEGV тощо) із автоматичним збереженням інформації про збій у базу даних SQLite.
