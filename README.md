# Lab 05 — Pipe + Thread + Ctrl+C (checkpointos, egyszerű)

Ez a labor direkt **nagyon egyszerű**, és három dolgot gyakorol:

1) `pipe()` + `fork()` (gyerek folyamat üzen a szülőnek)
2) `pthread_cond_t` (a worker szál nem pörög, hanem alszik és felébred)
3) Ctrl+C (SIGINT) → kulturált leállítás (gyerek leáll, szál befejez, cleanup)

---

## Futtatás (Ubuntu / WSL)

Ha kell a fordító:
```bash
sudo apt update
sudo apt install -y build-essential
```

Fordítás:
```bash
make
```

Futtatás:
```bash
./lab05
```

Leállítás:
- nyomj **Ctrl+C**-t

---

# Checkpoint 1 — Működik a fork+pipe?

Indítsd el a programot (`./lab05`).

Ezt kell látnod kb. másodpercenként:

- `[PARENT] received: SENSOR temperature=23C` (vagy hasonló)
- `[PARENT] received: SENSOR temperature=24C`
- ...

**Mit jelent ez?**
- a **child** ír a pipe-ba
- a **parent** olvas a pipe-ból

✅ Ha ezt látod, Checkpoint 1 kész.

---

# Checkpoint 2 — A worker tényleg “alszik” és felébred?

A program két helyen ír ki:

- a parent: `[PARENT] received: ...`
- a worker thread: `[WORKER] got: ...`

Ezt kell látnod:

- minden bejövő sorra a worker is reagál:
  - `[WORKER] got: SENSOR temperature=24C`

**Mit jelent ez?**
- a worker nem olvas pipe-ot
- a worker a parenttől kapja meg az üzenetet
- ha nincs új üzenet, a worker `pthread_cond_wait`-tel vár (nem pörög)

✅ Ha ezt látod, Checkpoint 2 kész.

---

# Checkpoint 3 — Ctrl+C után “szép” a leállás?

Nyomj Ctrl+C-t futás közben.

Ezt kell látnod a végén:

- `[MAIN] Ctrl+C received, shutting down...`
- `[MAIN] shutdown complete.`

**Mit jelent ez?**
- SIGINT beállít egy `running = 0` flaget
- a parent leállítja a gyereket (SIGTERM + wait)
- felébreszti a workert (`pthread_cond_broadcast`), hogy ki tudjon lépni
- `pthread_join` megvárja a thread végét

✅ Ha ezt látod, Checkpoint 3 kész.

---

## Leadandó
- a kész `main.c`

## Rövid kérdések (opcionális, 1-2 mondat)
1) Miért nem a worker olvas a pipe-ból?
2) Miért kell `pthread_cond_broadcast` leállításkor?
3) Miért kell `waitpid` a gyerekre?