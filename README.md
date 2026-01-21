# Lab 05 — Egyszerű Pipe + Thread + Ctrl+C

Ez a labor **szándékosan egyszerű**. Nem kell `man`, nem kell Valgrind.

## Mit tanulsz?

1) **pipe()**: két folyamat között adatküldés
2) **pthread_cond_t**: a szál tud "aludni", amíg nincs új adat (nincs busy-wait)
3) **SIGINT (Ctrl+C)**: kulturált leállítás

## A program felépítése (emberi nyelven)

- **Szülő folyamat (parent)**
  - létrehoz egy pipe-ot
  - `fork()`-kal elindít egy **gyerek** folyamatot
  - a pipe-ból olvas
  - az olvasott sort átadja a worker szálnak

- **Gyerek folyamat (child)**
  - 1 mp-enként küld egy sort a pipe-ba

- **Worker szál (thread)**
  - vár, amíg van új sor
  - ha kap sort, kiírja

- **Ctrl+C**
  - a program leállítja a gyerek folyamatot
  - felébreszti a workert
  - megvárja, míg minden befejeződik

## Futtatás (Ubuntu/WSL)

Telepítés (ha kell):

```bash
sudo apt update
sudo apt install -y build-essential
```

Fordítás és futtatás:

```bash
make
./lab05
```

Leállítás: **Ctrl+C**

## Mit kell beadni?

- A kész `main.c`

## Feladat (nagyon rövid)

1. Indítsd el.
2. Figyeld meg, hogy a worker soronként kap adatot.
3. Nyomj Ctrl+C-t, és nézd meg, hogy kiírja: `shutdown complete`.  
