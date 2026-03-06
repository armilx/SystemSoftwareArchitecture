# Практична робота №3

**Тема:** Дослідження обмежень ресурсів у середовищі Docker

## Завдання 3.1

```bash
ubuntu@8fe66291b225:/$ ulimit -n
1048576
ubuntu@8fe66291b225:/$ ulimit -aS | grep "open files"
open files                          (-n) 1048576
ubuntu@8fe66291b225:/$ ulimit -aH | grep "open files"
open files                          (-n) 1048576
ubuntu@8fe66291b225:/$ ulimit -n 3000
ubuntu@8fe66291b225:/$ ulimit -aS | grep "open files"
open files                          (-n) 3000
ubuntu@8fe66291b225:/$ ulimit -aH | grep "open files"
open files                          (-n) 3000
ubuntu@8fe66291b225:/$ ulimit -n 3001                       
bash: ulimit: open files: cannot modify limit: Operation not permitted
ubuntu@8fe66291b225:/$ ulimit -n 2000
ubuntu@8fe66291b225:/$ ulimit -n
2000
ubuntu@8fe66291b225:/$ ulimit -aS | grep "open files"
open files                          (-n) 2000
ubuntu@8fe66291b225:/$ ulimit -aH | grep "open files"
open files                          (-n) 2000
ubuntu@8fe66291b225:/$ ulimit -n 3000
bash: ulimit: open files: cannot modify limit: Operation not permitted
```
## Завдання 3.2

### Код

```c
/* open file limit */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int opened_files = 0;
    int fds[5000]; 

    printf("Starting file open test...\n");
    
    for (;;) {
        int current_fd = open("/dev/null", O_RDONLY);
        
        if (current_fd < 0) {
            printf("System limit reached! Successfully opened %d files.\n", opened_files);
            break; 
        }
        
        fds[opened_files] = current_fd;
        opened_files++;
    }

    sleep(1);

    for (int i = 0; i < opened_files; i++) {
        close(fds[i]);
    }

    return 0;
}
```

### Компіляція та запуск

```bash
root@905de1e7d14f:/# gcc limit_test.c -o limit_test
root@905de1e7d14f:/# ulimit -n 1200
root@905de1e7d14f:/# perf stat -e task-clock,context-switches,page-faults,syscalls:sys_enter_openat ./limit_test
Starting file open test...
System limit reached! Successfully opened 1197 files.

 Performance counter stats for './limit_test':

              5.26 msec task-clock                       #    0.005 CPUs utilized             
                 1      context-switches                 #  190.144 /sec                      
                50      page-faults                      #    9.507 K/sec                     
              1200      syscalls:sys_enter_openat        #  228.173 K/sec                     

       1.010805931 seconds time elapsed

       0.000000000 seconds user
       0.006373000 seconds sys

```

За допомогою perf ми можемо бачити 1197 спроб використати системний виклик open.

## Завдання 3.3

### Код

```c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

FILE *log_file;

void on_limit_reached(int signum) {
    printf("\n[УВАГА] Перевищено ліміт розміру файлу (Отримано сигнал SIGXFSZ: %d).\n", signum);
    if (log_file != NULL) {
        fclose(log_file);
    }
    printf("Файл безпечно закрито. Завершення роботи.\n");
    exit(0);
}

int main() {
    signal(SIGXFSZ, on_limit_reached);

    log_file = fopen("my_dice.txt", "w");
    if (log_file == NULL) {
        perror("Не вдалося відкрити файл");
        return 1;
    }

    srand(time(NULL));

    while (1) {
        int current_roll = (rand() % 6) + 1; // Кубик від 1 до 6
        
        if (fprintf(log_file, "%d\n", current_roll) < 0) {
            break;
        }
    }

    fclose(log_file);
    return 0;
}
```

### Компіляція та запуск

```bash
root@905de1e7d14f:/# nano dice.c
root@905de1e7d14f:/# gcc dice.c -o dice 
root@905de1e7d14f:/# ulimit -f 10
root@905de1e7d14f:/# ./dice

root@905de1e7d14f:/# tail my_dice.txt
1
3
4
6
2
2
2
3
5
2
```

Під час тестування програма безперервно записувала згенеровані числа у файл. Як тільки розмір файлу `my_dice.txt` досяг встановленого ліміту (10 блоків), операційна система надіслала процесу сигнал `SIGXFSZ`. 
Завдяки попередньо зареєстрованому обробнику сигналів, програма успішно перехопила цей сигнал, вивела відповідне повідомлення, безпечно закрила потік запису (`fclose`) та штатно завершила виконання. Це демонструє важливість коректної обробки системних сигналів для стабільності програмного забезпечення при роботі з обмеженими ресурсами ОС.

## Завдання 3.4

### Код

```c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

void catch_cpu_limit(int sig) {
    printf("\nTIME IS UP! Received SIGXCPU signal\n");
    exit(0);
}
int is_duplicate(int number, int array[], int current_size) {
    for (int i = 0; i < current_size; i++) {
        if (array[i] == number) {
            return 1;
        }
    }
    return 0;
}

int main() {
    signal(SIGXCPU, catch_cpu_limit);
    srand(time(NULL));

    int lotto7[7];
    int lotto6[6];

    while (1) {
        for (int i = 0; i < 7; ) {
            int random_num = (rand() % 49) + 1;
            if (is_duplicate(random_num, lotto7, i) == 0) {
                lotto7[i] = random_num;
                i++;
            }
        }
        for (int i = 0; i < 6; ) {
            int random_num = (rand() % 36) + 1;
            if (is_duplicate(random_num, lotto6, i) == 0) {
                lotto6[i] = random_num;
                i++;
            }
        }
        printf("Lotto [7 of 49]: ");
        for (int i = 0; i < 7; i++) printf("%d ", lotto7[i]);
        
        printf(" | Lotto [6 of 36]: ");
        for (int i = 0; i < 6; i++) printf("%d ", lotto6[i]);
        
        printf("\n");
    }

    return 0;
}
}
```

### Компіляція та запуск

```bash
root@905de1e7d14f:/# nano simple_lotto.c
root@905de1e7d14f:/#gcc simple_lotto.c -o simple_lotto
root@905de1e7d14f:/# ulimit -St 1
root@905de1e7d14f:/# ./simple_lotto
Lotto [7 of 49]: 13 12 40 23 21 9 1  | Lotto [6 of 36]: 6 12 3 34 14 22 
Lotto [7 of 49]: 23 42 2 5 4 19 39  | Lotto [6 of 36]: 30 36 4 29 21 22 
Lotto [7 of 49]: 33 2 17 49 41 39 20  | Lotto [6 of 36]: 13 28 3 25 21 16 
Lotto [7 of 49]: 11 31 48 38 28 49 42  | Lotto [6 of 36]: 10 34 12 28 7 32 
Lotto [7 of 49]: 6 37 15 43 36 42 40  | Lotto [6 of 36]: 26 22 21 34 24 9 
Lotto [7 of 49]: 25 23 11 40 4 10 28  | Lotto [6 of 36]: 23 5 7 12 2 34 
Lotto [7 of 49]: 38 4 5 15 40 19 8  | Lotto [6 of 36]: 33 26 17 21 31 18 
Lotto [7 of 49]: 28 5 36 30 1 46 25  | Lotto [6 of 36]: 11 33 3 34 2 25 
Lotto [7 of 49]: 42 46 30 35 6 34 39  | Lotto [6 of 36]: 13 26 33 21 22 2 
[...]
Lotto [7 of 49]: 6 2 42 46 13 27 16  | Lotto [6 of 36]: 25 21 22 13 4 16 
Lotto [7 of 49]: 15 12 35 23 4 45 31  | Lotto [6 of 36]: 35 14 10 17 16 18 
Lotto [7 of 49]: 39 47 40 48 4 46 45  | Lotto [6 of 36]: 1 28 5 6 16 26 
Lotto [7 of 49]: 20 11 34 32 31 45 22  | Lotto [6 of 36]: 34 3 35 20 
TIME IS UP! Received SIGXCPU signal

```
## Завдання 3.5

### Код

```c
/* copy */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Program needs at least two arguments\n");
		return 1;
	}

	char *src = argv[1];
	char *dst = argv[2];

	FILE *in = fopen(src, "rb");
	if (!in) {
		fprintf(stderr, "Cannot open file %s for reading\n", src);
		return 1;
	}

	FILE *out = fopen(dst, "wb");
	if (!out) {
		fprintf(stderr, "Cannot open file %s for writing\n", dst);
		fclose(in);
		return 1;
	}

	char buffer[BUF_SIZE];

	size_t nread;
	while ((nread = fread(buffer, 1, BUF_SIZE, in)) > 0) {

		size_t nwritten = fwrite(buffer, 1, nread, out);

		if (nwritten < nread) {
			if (ferror(out)) {
				if (errno == EFBIG) {
					fprintf(stderr, "File size limit exceeded while writing\n");
				} else {
					fprintf(stderr, "Write error: %s\n", strerror(errno));
				}
			}
			fclose(in);
			fclose(out);
			return 1;
		}
	}

	if (ferror(in)) {
		fprintf(stderr, "Read error: %s\n", strerror(errno));
	}

	fclose(in);
	fclose(out);
	return 0;
}
```

### Компіляція та запуск

```bash
root@905de1e7d14f:/# nano copy.c   
root@905de1e7d14f:/# gcc copy.c -o copy
root@905de1e7d14f:/# ./copy
Program need two arguments
root@905de1e7d14f:/# ./copy fake_file.txt output.txt
Cannot open file fake_file.txt for reading
root@905de1e7d14f:/# ulimit -f 1
root@905de1e7d14f:/# ./copy /etc/os-release my_copied_file.txt
File copied successfully!

```
## Завдання 3.6

### Код

```c
/* stack_test */
#include <stdio.h>

unsigned long long fact(unsigned long long n) {
	return n * fact(n - 1);
}

int main() {
	unsigned long long n = 1000000;
	printf("calculating factorial of %llu...\n", n);

	unsigned long long result = fact(n);

	printf("result: %llu\n", result);
	return 0;
}
```

### Компіляція та запуск

```bash
root@905de1e7d14f:/# nano stack_test.c
root@905de1e7d14f:/# gcc stack_test.c -o stack_test
root@905de1e7d14f:/# ./stack_test
Starting stack overflow test...
Recursion depth reached: 50
Recursion depth reached: 100
Recursion depth reached: 150
Recursion depth reached: 200
Recursion depth reached: 250
Recursion depth reached: 300
[...]
Recursion depth reached: 19200
Recursion depth reached: 19250
Recursion depth reached: 19300
Recursion depth reached: 19350
Recursion depth reached: 19400
Segmentation fault (core dumped)
```

## Завдання за варіантом 2

### Постановка

```
2. Встановити обмеження на кількість відкритих сокетів (ulimit -n) та запустити серверну програму.
```
### Код

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    printf("Starting simple server program...\n");

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create main server socket");
        return 1;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Bind failed");
        return 1;
    }

    listen(server_socket, 5);
    printf("Server is listening on port 8080.\n");
    printf("Trying to open as many sockets as possible to test the limit...\n");

    int total_sockets = 1;

    while (1) {
        int dummy_socket = socket(AF_INET, SOCK_STREAM, 0);
        
        if (dummy_socket < 0) {
            printf("\n--- SYSTEM LIMIT REACHED ---\n");
            printf("Total sockets opened by this program: %d\n", total_sockets);
            perror("Reason for failure");
            break;
        }
        
        total_sockets++;
    }

    close(server_socket);
    return 0;
}
```
### Компіляція та запуск
```bash
root@905de1e7d14f:/# nano server_limit.c
root@905de1e7d14f:/# gcc server_limit.c -o server_limit
root@905de1e7d14f:/# ulimit -n 35
root@905de1e7d14f:/# ./server_limit
Starting simple server program...
Server is listening on port 8080.
Trying to open as many sockets as possible to test the limit...

--- SYSTEM LIMIT REACHED ---
Total sockets opened by this program: 32
Reason for failure: Too many open files
```
(Буде відкрито 32 сокети, бо ще 3 дескриптори забирає сам термінал для виводу тексту на екран: 32 + 3 = 35).
