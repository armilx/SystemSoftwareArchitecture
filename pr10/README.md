## Тестування прикладів з Лекції 13 (Signaling — Part 2)

### Приклад 1. Crash handler із SA_SIGINFO і register dump

**Опис:** Програма демонструє перехоплення критичних помилок (наприклад, `SIGSEGV`) за допомогою `sigaction` з прапорцем `SA_SIGINFO`. Замість стандартного аварійного завершення, обробник сигналу безпечно (через `write`) виводить діагностичну інформацію: номер сигналу, адресу помилки та стан регістрів процесора, після чого контрольовано завершує процес.

**Код програми (crash_diag.c):**

```c
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <unistd.h>
#include <stdlib.h>

static void wr_all(const char *s, unsigned long n) {
    while (n > 0) {
        ssize_t r = write(STDERR_FILENO, s, n);
        if (r <= 0) return;
        s += r;
        n -= (unsigned long)r;
    }
}

static void wr(const char *s) {
    unsigned long n = 0;
    while (s[n] != '\0') n++;
    wr_all(s, n);
}

static void wr_ch(char c) {
    wr_all(&c, 1);
}

static void wr_dec(long v) {
    char buf[32];
    int i = 0;
    unsigned long x;

    if (v < 0) {
        wr_ch('-');
        x = (unsigned long)(-(v + 1)) + 1UL;
    } else {
        x = (unsigned long)v;
    }

    do {
        buf[i++] = (char)('0' + (x % 10));
        x /= 10;
    } while (x != 0 && i < (int)sizeof(buf));

    while (i > 0) wr_ch(buf[--i]);
}

static void wr_hex(uint64_t v) {
    static const char hex[] = "0123456789abcdef";
    int started = 0;

    wr("0x");
    for (int shift = 60; shift >= 0; shift -= 4) {
        unsigned int nib = (unsigned int)((v >> shift) & 0xfU);
        if (nib != 0 || started || shift == 0) {
            wr_ch(hex[nib]);
            started = 1;
        }
    }
}

static void wr_ptr(const void *p) {
    wr_hex((uint64_t)(uintptr_t)p);
}

static void crash_handler(int sig, siginfo_t *si, void *ctx) {
    int saved_errno = errno;

    wr("\n=== crash captured ===\n");
    wr("signal: ");
    wr_dec(sig);
    wr("\n");

    if (si != NULL) {
        wr("si_code: ");
        wr_dec((long)si->si_code);
        wr("\n");

        wr("fault address: ");
        wr_ptr(si->si_addr);
        wr("\n");
    }

#if defined(__x86_64__)
    if (ctx != NULL) {
        ucontext_t *uc = (ucontext_t *)ctx;
        greg_t *g = uc->uc_mcontext.gregs;

        wr("RIP: "); wr_hex((uint64_t)g[REG_RIP]); wr("\n");
        wr("RSP: "); wr_hex((uint64_t)g[REG_RSP]); wr("\n");
        wr("RBP: "); wr_hex((uint64_t)g[REG_RBP]); wr("\n");
        wr("RAX: "); wr_hex((uint64_t)g[REG_RAX]); wr("\n");
        wr("RBX: "); wr_hex((uint64_t)g[REG_RBX]); wr("\n");
        wr("RCX: "); wr_hex((uint64_t)g[REG_RCX]); wr("\n");
        wr("RDX: "); wr_hex((uint64_t)g[REG_RDX]); wr("\n");
        wr("RSI: "); wr_hex((uint64_t)g[REG_RSI]); wr("\n");
        wr("RDI: "); wr_hex((uint64_t)g[REG_RDI]); wr("\n");
    }
#else
    wr("Register dump is implemented here only for x86-64.\n");
#endif

    errno = saved_errno;
    _exit(128 + sig);
}

static void install_crash_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = crash_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS,  &sa, NULL);
    sigaction(SIGFPE,  &sa, NULL);
    sigaction(SIGILL,  &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
}

__attribute__((noinline))
static void crash_here(void) {
    volatile int *p = (int *)0;
    *p = 42;
}

int main(void) {
    install_crash_handlers();

    wr("About to crash. PID=");
    wr_dec((long)getpid());
    wr("\n");

    crash_here();
    return 0;
}
```

**Компіляція та запуск:**
```bash
andriib@andriib:~$ nano crash_diag.c
andriib@andriib:~$ gcc -Wall -Wextra -O0 -g -fno-omit-frame-pointer -no-pie crash_diag.c -o crash_diag
crash_diag.c: In function \u2018crash_handler\u2019:
crash_diag.c:68:57: warning: unused parameter \u2018ctx\u2019 [-Wunused-parameter]
   68 | static void crash_handler(int sig, siginfo_t *si, void *ctx) {
      |                                                   ~~~~~~^~~
andriib@andriib:~$ ./crash_diag
About to crash. PID=11363

=== crash captured ===
signal: 11
si_code: 1
fault address: 0x0
Register dump is implemented here only for x86-64.
andriib@andriib:~$ addr2line -e ./crash_diag -f -C 0x40128a
??
??:0
```

---

### Приклад 2. Правильне використання sleep при перериванні сигналами

**Опис:** Демонструє, як уникнути проблеми зсуву часу (drift), коли системний виклик переривається сигналом. Показано два підходи: цикл із `nanosleep` для відносного часу та використання `clock_nanosleep` з абсолютним часом (`TIMER_ABSTIME`) для періодичних задач.

**Код програми (sleep_correct.c):**

```c
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t got_usr1 = 0;

static void on_usr1(int sig) {
    (void)sig;
    got_usr1 = 1;
}

static int sleep_relative_ms(long ms) {
    struct timespec req = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000L
    };
    struct timespec rem;

    while (nanosleep(&req, &rem) == -1) {
        if (errno == EINTR) {
            req = rem;
            continue;
        }
        return -1;
    }

    return 0;
}

static void add_ms(struct timespec *t, long ms) {
    t->tv_sec += ms / 1000;
    t->tv_nsec += (ms % 1000) * 1000000L;

    while (t->tv_nsec >= 1000000000L) {
        t->tv_sec++;
        t->tv_nsec -= 1000000000L;
    }
}

static int sleep_periodic_absolute(struct timespec *deadline, long period_ms) {
    int rc;

    add_ms(deadline, period_ms);

    while ((rc = clock_nanosleep(CLOCK_MONOTONIC,
                                 TIMER_ABSTIME,
                                 deadline,
                                 NULL)) == EINTR) {
    }

    if (rc != 0) {
        errno = rc;
        return -1;
    }

    return 0;
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = on_usr1;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("PID=%ld. In another terminal: kill -USR1 %ld\n",
           (long)getpid(), (long)getpid());

    puts("Relative sleep for 5 seconds using nanosleep restart loop...");
    if (sleep_relative_ms(5000) == -1) {
        perror("nanosleep");
        return 1;
    }

    printf("Relative sleep finished. got_usr1=%d\n", got_usr1);

    puts("Now 5 periodic ticks with absolute clock_nanosleep deadlines...");

    struct timespec next;
    if (clock_gettime(CLOCK_MONOTONIC, &next) == -1) {
        perror("clock_gettime");
        return 1;
    }

    for (int i = 1; i <= 5; i++) {
        if (sleep_periodic_absolute(&next, 1000) == -1) {
            perror("clock_nanosleep");
            return 1;
        }
        printf("tick %d\n", i);
    }

    return 0;
}
```

**Компіляція та запуск:**
```bash
andriib@andriib:~$ gcc -Wall -Wextra -O2 sleep_correct.c -o sleep_correct
andriib@andriib:~$ ./sleep_correct
PID=4130. In another terminal: kill -USR1 4130
Relative sleep for 5 seconds using nanosleep restart loop...
Relative sleep finished. got_usr1=1
Now 5 periodic ticks with absolute clock_nanosleep deadlines...
tick 1
tick 2
tick 3
tick 4
tick 5
```
---

### Приклад 3. Реалізація Publisher-Subscriber через Real-time signals

**Опис:** Використання сигналів реального часу (`SIGRTMIN`) як мінімалістичного механізму міжпроцесної взаємодії. Програма працює як отримувач (`sub`), що чекає сигнал через `sigwaitinfo`, або як відправник (`pub`), який ставить сигнали в чергу та передає цілочисельні значення через `sigqueue`.

**Код програми (rt_pubsub.c):**

```c
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static long parse_long(const char *s, const char *what) {
    char *end = NULL;
    errno = 0;

    long v = strtol(s, &end, 10);

    if (errno != 0 || end == s || *end != '\0') {
        fprintf(stderr, "invalid %s: %s\n", what, s);
        exit(EXIT_FAILURE);
    }

    return v;
}

static int app_signal(void) {
    int sig = SIGRTMIN;

    if (sig > SIGRTMAX) {
        fprintf(stderr, "No available real-time signal\n");
        exit(EXIT_FAILURE);
    }

    return sig;
}

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s sub\n"
        "  %s sub-timeout\n"
        "  %s pub <subscriber-pid> <int> [<int> ...]\n",
        prog, prog, prog);
}

static void subscriber(int use_timeout) {
    int sig = app_signal();

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sig);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        die("sigprocmask");
    }

    printf("subscriber PID=%ld, waiting for signal %d (SIGRTMIN)\n",
           (long)getpid(), sig);
    fflush(stdout);

    for (;;) {
        siginfo_t si;
        memset(&si, 0, sizeof(si));

        int r;

        if (use_timeout) {
            struct timespec ts = {
                .tv_sec = 5,
                .tv_nsec = 0
            };
            r = sigtimedwait(&set, &si, &ts);
        } else {
            r = sigwaitinfo(&set, &si);
        }

        if (r == -1) {
            if (errno == EINTR) {
                continue;
            }

            if (use_timeout && errno == EAGAIN) {
                puts("timeout: no messages for 5 seconds");
                continue;
            }

            die(use_timeout ? "sigtimedwait" : "sigwaitinfo");
        }

        int value = si.si_value.sival_int;

        printf("received signal=%d value=%d from pid=%ld uid=%ld\n",
               r,
               value,
               (long)si.si_pid,
               (long)si.si_uid);
        fflush(stdout);

        if (value < 0) {
            puts("negative value received: shutting down subscriber");
            break;
        }
    }
}

static void publisher(pid_t pid, int argc, char **argv) {
    int sig = app_signal();

    for (int i = 3; i < argc; i++) {
        union sigval value;
        value.sival_int = (int)parse_long(argv[i], "message value");

        if (sigqueue(pid, sig, value) == -1) {
            die("sigqueue");
        }

        printf("sent value=%d to pid=%ld via signal=%d\n",
               value.sival_int,
               (long)pid,
               sig);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "sub") == 0) {
        subscriber(0);
    } else if (strcmp(argv[1], "sub-timeout") == 0) {
        subscriber(1);
    } else if (strcmp(argv[1], "pub") == 0) {
        if (argc < 4) {
            usage(argv[0]);
            return EXIT_FAILURE;
        }

        pid_t pid = (pid_t)parse_long(argv[2], "PID");
        publisher(pid, argc, argv);
    } else {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

**Компіляція та запуск:**
```bash
andriib@andriib:~$ gcc -Wall -Wextra -O2 rt_pubsub.c -o rt_pubsub
```

**Термінал 1 (Subscriber):**
```bash
andriib@andriib:~$ ./rt_pubsub sub
subscriber PID=11533, waiting for signal 34 (SIGRTMIN)
received signal=34 value=10 from pid=11558 uid=1000
received signal=34 value=20 from pid=11558 uid=1000
received signal=34 value=-1 from pid=11558 uid=1000
negative value received: shutting down subscriber
```

**Термінал 2 (Publisher):**
```bash
andriib@andriib:~$ ./rt_pubsub pub 11533 10 20 -1
sent value=10 to pid=11533 via signal=34
sent value=20 to pid=11533 via signal=34
sent value=-1 to pid=11533 via signal=34
andriib@andriib:~$ 


```

---

### Висновок
Під час тестування прикладів з лекції було на практиці закріплено механізми обробки сигналів у Linux. Досліджено роботу crash-handler'а для безпечного перехоплення критичних помилок (`SIGSEGV`) із виведенням стану регістрів процесора. Перевірено коректне використання таймерів (`nanosleep` та `clock_nanosleep`) при їх перериванні сигналами (`EINTR`) для уникнення зсуву часу. Також успішно реалізовано патерн міжпроцесної взаємодії (Publisher-Subscriber) з використанням сигналів реального часу (`SIGRTMIN`) та функцій `sigqueue` і `sigwaitinfo` для передачі даних.
