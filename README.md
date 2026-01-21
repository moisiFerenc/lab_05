# Lab 05 - Pipe + Thread + Signal kezeles

## Attekintes

Ez a labor a kovetkezo Unix/Linux programozasi temakat gyakoroltatja:

| Tema | Mit tanulunk? |
|------|---------------|
| `pipe()` | Anonim pipe letrehozasa processzek kozti kommunikaciora |
| `fork()` | Gyerek process letrehozasa |
| `read()/write()` | Pipe-on keresztuli adatatvitel |
| `pthread_cond_t` | Condition variable (producer-consumer minta) |
| `signal()` | SIGINT (Ctrl+C) kezelese |
| Cleanup | Szalak join, gyerek waitpid, eroforras felszabaditas |

---

## Feladat

A `main.c` fajlban **11 darab TODO** van, amiket ki kell tolteni.

A program egy homerseklet monitor rendszer:
- Gyerek process (szenzor): masodpercenkent ir a pipe-ba
- Szulo process: olvassa a pipe-ot es atadja a worker thread-nek
- Worker thread: feldolgozza az adatokat
- Ctrl+C: kulturalt leallitas (minden komponens rendben lezar)

---

## Telepites (Ubuntu)

```bash
sudo apt update
sudo apt install -y build-essential valgrind
```

---

## Forditas es futtatas

```bash
make        # Forditas
make run    # Futtatas
# Leallitas: Ctrl+C
```

---

## TODO lista

| # | Hely | Mit kell csinalni |
|---|------|-------------------|
| 1 | `sigint_handler()` | Allitsd `running`-ot 0-ra |
| 2 | `sensor_process()` | Iras a pipe-ba `write()`-tal |
| 3 | `worker_thread()` | Varakozas `pthread_cond_wait()`-tel |
| 4 | `main()` | SIGINT kezelo beallitasa `signal()`-lal |
| 5 | `main()` | Pipe letrehozasa `pipe()`-pal |
| 6 | `main()` | Gyerek process letrehozasa `fork()`-kal |
| 7 | `main()` | Gyerekben: zard be az olvaso veget |
| 8 | `main()` | Szuloben: zard be az iro veget |
| 9 | `main()` | Adat atadasa a workernek (mutex + cond_signal) |
| 10 | `main()` | Gyerek leallitasa: `kill()` + `waitpid()` |
| 11 | `main()` | Worker ebresztese: `pthread_cond_broadcast()` |

---

## Ellenorzesi pontok

### 1. Mukodik a pipe?

Ha jol csinaltad a TODO 2, 5, 6, 7, 8-at:
```
[SZENZOR] Elindult (PID: 12345)
[MAIN] Beolvasva: TEMP 23C
[MAIN] Beolvasva: TEMP 25C
```

### 2. Mukodik a condition variable?

Ha jol csinaltad a TODO 3, 9-et:
```
[MAIN] Beolvasva: TEMP 24C
[WORKER] Feldolgozva: TEMP 24C
```

### 3. Mukodik a kulturalt leallitas?

Ha jol csinaltad a TODO 1, 4, 10, 11-et, Ctrl+C utan:
```
[MAIN] Leallitas megkezdese...
[MAIN] Gyerek process leallitasa...
[MAIN] Gyerek process leallt
[MAIN] Worker thread bevarasa...
[WORKER] Szal leallt

========================================
  LEALLITAS SIKERES!
========================================
```

---

## Hasznos fuggvenyek

### Pipe

```c
int pipe_fd[2];
pipe(pipe_fd);           // Letrehoz egy pipe-ot
// pipe_fd[0] = olvaso veg
// pipe_fd[1] = iro veg

write(pipe_fd[1], buf, len);  // Iras
read(pipe_fd[0], buf, len);   // Olvasas
close(pipe_fd[0]);            // Bezaras
```

### Fork

```c
pid_t pid = fork();
if (pid < 0) { /* hiba */ }
if (pid == 0) { /* gyerek */ }
if (pid > 0) { /* szulo, pid = gyerek PID */ }
```

### Signal

```c
signal(SIGINT, handler_fn);   // Ctrl+C kezelo beallitasa
kill(pid, SIGTERM);           // Signal kuldese
waitpid(pid, NULL, 0);        // Gyerek bevarasa
```

### Condition Variable

```c
pthread_mutex_lock(&mtx);
while (!feltetel) {
    pthread_cond_wait(&cond, &mtx);  // Var es elengedi a mutex-et
}
pthread_mutex_unlock(&mtx);

// Masik szalban:
pthread_mutex_lock(&mtx);
feltetel = 1;
pthread_cond_signal(&cond);   // Egy szalat ebreszt
pthread_mutex_unlock(&mtx);

pthread_cond_broadcast(&cond); // Minden szalat ebreszt
```

---

## Valgrind ellenorzes

```bash
make valgrind
```

Helyes kimenet (Ctrl+C utan):
```
==12345== LEAK SUMMARY:
==12345==    definitely lost: 0 bytes in 0 blocks
```

---

## Megoldas

A `solution/main.c` fajlban megtalalhato a teljes megoldas.
Csak akkor nezd meg, ha tenyleg elakadtal!

---

## Leadando

- A kitoltott `main.c` fajl
- Screenshot a mukodo programrol (mindharom ellenorzesi pont)
