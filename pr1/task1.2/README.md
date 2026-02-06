# Практична робота №1 , завдання 1.2
Виконав: Білоус Андрій Олегович

Завдання: Дослідіть, які бібліотеки доступні у вашій системі.

### 1. Перевірка залежностей програм (утиліта ldd)
Перевірено бібліотеки для системних утиліт `ls` та `gcc`.

#### 1.1. Аналіз /bin/ls
Команда: `ldd /bin/ls`
Резульат:
<img width="1390" height="196" alt="image" src="https://github.com/user-attachments/assets/5ed2b584-bee4-49dd-ad1a-ccf4b0073c0c" />

#### 1.2. Аналіз /usr/bin/gcc
Команда: `ldd /usr/bin/gcc`
Результат:
<img width="1201" height="130" alt="image" src="https://github.com/user-attachments/assets/e71d1593-619b-49b8-a6f7-15cdd9a68704" />

#### 2. Пошук math-бібліотеки

#### 2.1 Пошук за маскою (згідно завдання)
Команда: `find /usr/lib -name "*math*`
Результат:
<img width="1296" height="174" alt="image" src="https://github.com/user-attachments/assets/9d53a402-8048-4ca9-9bb2-41ca282725a8" />

#### 2.2. Уточнення розташування
Команда: `find /usr/lib -name "libm.so.6"`
Результат:
<img width="825" height="62" alt="image" src="https://github.com/user-attachments/assets/e4ea59a4-3710-4b29-884f-65f79b77887e" />

### 3. Перевірте символи бібліотеки за допомогою nm або objdump

#### 3.1 Аналіз nm -D /usr/lib/libm.so | grep erf
Команда: `nm -D /usr/lib/libm.so | grep erf`
Результат: 
<img width="1195" height="1345" alt="image" src="https://github.com/user-attachments/assets/2e6d06a2-ebe9-4974-94f7-1b5a725ebd56" />

### 4 Проаналізуйте залежності бібліотек за допомогою ldd або objdump -p.
#### 4.1 Аналіз за допомогою ldd 
Команда: `ldd /usr/lib/aarch64-linux-gnu/libm.so.6`

### 5 Скрипт автоматичного аналізу (Bash)
#### Був написаний скрипт `scan_libs.sh`, який сканує директорію `/usr/lib`, знаходить усі .so файли та шукає в них функції `sin`, `cos`, `exp`.

Код скрипта: Див. файл `scan_libs.sh` 
Приклад роботи скрипта:
<img width="1271" height="1061" alt="image" src="https://github.com/user-attachments/assets/52b87550-442c-4ac3-b5aa-cc4ad737a3be" />

