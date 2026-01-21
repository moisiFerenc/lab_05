# Lab 05 — Pipe + Condition Variable + SIGINT (Lab 04.5)

Ez a mini-labor arra szolgál, hogy a vizsga előtt gyakorold azokat a témákat, amik a lab_02/03/04-ben nem (vagy nem elég direkt módon) jelentek meg:

- `pipe()` + `fork()` (alap IPC, `read`/`write`, fd-k lezárása)
- `pthread_cond_t` (producer–consumer, busy-wait nélkül)
- `SIGINT` (Ctrl+C) és "szép" leállítás (thread join + child process kill/wait)

## Feladat röviden

1. A szülő folyamat létrehoz egy pipe-ot, majd `fork()`-ol.
2. A gyerek folyamat 1 másodpercenként generál egy sort és a pipe-ba írja (pl. `AAPL 150`).
3. A szülő a pipe-ból olvas, és egy **egyelemű mailbox-ba** teszi az utolsó sort.
4. Egy worker thread `pthread_cond_wait`-tel vár, amíg van új sor, majd feldolgozza (kiírja).
5. Ctrl+C (SIGINT) esetén a program leáll:
   - `running = 0`
   - child process leállítása (`kill` + `waitpid`)
   - worker felébresztése (`pthread_cond_broadcast`), `pthread_join`
   - fd-k és szinkron primitívek takarítása

## Követelmények

- Busy-wait tilos (nincs `while(empty){}` pörgés).
- `pthread_cond_wait`-et **while** feltétellel használd (spurious wakeup).
- Pipe végeket helyesen zárd le (`close`).

## Build & Run (Ubuntu/WSL)

```bash
sudo apt update
sudo apt install -y build-essential valgrind

make
make run
```

Valgrind:

```bash
make valgrind
```

Hasznos man oldalak:

```bash
man 2 pipe
man 2 fork
man 2 read
man 2 write
man 3 pthread_cond_wait
man 7 signal
```

---

## Leadandó

- A kész `main.c` (a TODO-k kitöltve)

## Tipp

Ha nem működik a leállítás:
- Nézd meg, hogy Ctrl+C-re tényleg átáll-e a `running` flag.
- A worker szálat ébreszted-e `pthread_cond_broadcast`-tel.
- A child process megkapja-e a SIGTERM-et.
