# Lab 05 — Pipe + Thread + Signal kezelés

## Áttekintés

Ez a labor a következő Unix/Linux programozási témákat gyakoroltatja:

| Téma | Mit tanulunk? |
|------|---------------|
| `pipe()` | Anonim pipe létrehozása processzek közti kommunikációra |
| `fork()` | Gyerek process létrehozása |
| `read()/write()` | Pipe-on keresztüli adatátvitel |
| `pthread_cond_t` | Condition variable (producer-consumer minta) |
| `signal()` | SIGINT (Ctrl+C) kezelése |
| Cleanup | Szálak join, gyerek waitpid, erőforrás felszabadítás |

---

## A program felépítése

```
┌─────────────────────┐          ┌─────────────────────────────────────┐
│   SZENZOR PROCESS   │          │           MAIN PROCESS              │
│     (gyerek)        │          │                                     │
│                     │   PIPE   │   ┌─────────┐      ┌──────────┐    │
│  while(1) {         │ ──────►  │   │  MAIN   │ cond │  WORKER  │    │
│    write("TEMP")    │          │   │  LOOP   │ ───► │  THREAD  │    │
│    sleep(1)         │          │   │(olvas)  │  var │(feldolg.)│    │
│  }                  │          │   └─────────┘      └──────────┘    │
└─────────────────────┘          └─────────────────────────────────────┘
         │                                        │
         │              Ctrl+C (SIGINT)           │
         └────────────────────────────────────────┘
                    Kulturált leállítás
```

---

## Telepítés (Ubuntu)

```bash
# Build eszközök telepítése (ha még nincs)
sudo apt update
sudo apt install -y build-essential valgrind

# Fordítás
make

# Futtatás
make run

# Leállítás: Ctrl+C
```

---

## Checkpoint 1 — Működik a pipe?

### Mit kell látni?

Indítsd el a programot:
```bash
./lab05
```

Másodpercenként megjelenik:
```
[SZENZOR] Elindult (PID: 12345)
[MAIN] Beolvasva: TEMP 23C
[MAIN] Beolvasva: TEMP 25C
...
```

### Mit jelent ez?

- ✅ A **gyerek process** (szenzor) ír a pipe-ba
- ✅ A **szülő process** olvassa a pipe-ból
- ✅ A pipe működik!

---

## Checkpoint 2 — Működik a condition variable?

### Mit kell látni?

Minden beolvasott sorra a worker szál is reagál:
```
[MAIN] Beolvasva: TEMP 24C
[WORKER] Feldolgozva: TEMP 24C
```

### Mit jelent ez?

- ✅ A **main loop** a producer (új adatot tesz be)
- ✅ A **worker thread** a consumer (kiveszi és feldolgozza)
- ✅ Ha nincs adat, a worker `pthread_cond_wait()`-tel vár (nem pörög!)

### Miért fontos a condition variable?

```c
// ROSSZ megoldás (busy waiting):
while (!has_data) {
    // CPU 100%-on pörög...
}

// JÓ megoldás (condition variable):
while (!has_data) {
    pthread_cond_wait(&cond, &mtx);  // CPU alszik
}
```

---

## Checkpoint 3 — Működik a kulturált leállítás?

### Mit kell látni?

Nyomj **Ctrl+C**-t futás közben:
```
^C
[MAIN] Leállítás megkezdése...
[MAIN] Gyerek process leállítása (SIGTERM)...
[MAIN] Gyerek process leállt
[MAIN] Worker thread bevárása...
[WORKER] Szál leállt

========================================
  LEÁLLÍTÁS SIKERES!
========================================
```

### Mi történik ilyenkor?

1. **SIGINT handler** beállítja: `running = 0`
2. **Main loop** kilép a while ciklusból
3. **Gyerek process** kap egy SIGTERM-et, majd `waitpid()` bevárja
4. **Worker thread** felébresztése `pthread_cond_broadcast()`-tal
5. **pthread_join()** megvárja a thread végét
6. **Cleanup**: pipe lezárása, mutex/cond destroy

---

## Valgrind ellenőrzés

```bash
make valgrind
```

Futtasd, várj pár másodpercet, majd Ctrl+C. A helyes kimenet:
```
==12345== LEAK SUMMARY:
==12345==    definitely lost: 0 bytes in 0 blocks
==12345==    indirectly lost: 0 bytes in 0 blocks
```

---

## Fontos kérdések (gondolkodtató)

1. **Miért nem a worker thread olvas közvetlenül a pipe-ból?**
   > Mert így bemutatjuk a producer-consumer mintát. Valós alkalmazásban a main 
   > loop több forrásból is gyűjthet adatot, majd egy közös pufferbe teszi.

2. **Miért kell `pthread_cond_broadcast()` leállításkor?**
   > Mert lehet több worker thread is. A `broadcast` mindet felébreszti, 
   > míg a `signal` csak egyet. Leállításkor biztosak akarunk lenni, hogy 
   > minden szál értesül.

3. **Miért kell `waitpid()` a gyerekre?**
   > Hogy ne legyen zombie process. A kernel addig tárolja a gyerek kilépési
   > státuszát, amíg a szülő le nem kérdezi `waitpid()`-dal.

4. **Mi az a spurious wakeup és miért kell while ciklus a wait körül?**
   > A `pthread_cond_wait()` néha ok nélkül is visszatérhet. Ezért mindig 
   > újra ellenőrizzük a feltételt (`while (!has_data)`).

---

## Leadandó

- [x] A működő `main.c` fájl
- [x] Screenshot a futásról (Checkpoint 1-3 teljesítve)
- [x] Valgrind kimenet (nincs memory leak)

---

## Hasznos parancsok

```bash
# Futó processzek listázása
ps aux | grep lab05

# Process fa megjelenítése
pstree -p $$

# Szálak listázása
ps -T -p <PID>

# Signal küldése kézzel
kill -SIGINT <PID>
kill -SIGTERM <PID>
```

---

## Gyakori hibák

| Hiba | Megoldás |
|------|----------|
| `zombie process` | Mindig hívj `waitpid()`-t a gyerekre! |
| `deadlock` | Ne lockold a mutex-et kétszer ugyanabban a szálban! |
| `100% CPU` | Használj condition variable-t busy wait helyett! |
| `broken pipe` | Ellenőrizd, hogy mindkét véget bezártad-e a megfelelő helyen! |
| `race condition` | Minden megosztott változót védd mutex-szel! |
