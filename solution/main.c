/**
 * ============================================================================
 *  LAB 05 - Operációs Rendszerek Labor
 * ============================================================================
 * 
 *  TÉMA: Pipe + Thread + Signal kezelés
 * 
 *  Mit tanulunk meg ebben a laborban?
 *  ---------------------------------
 *  1. pipe()       - anonim pipe létrehozása processzek közti kommunikációra
 *  2. fork()       - gyerek process létrehozása
 *  3. read/write   - pipe-on keresztüli kommunikáció
 *  4. pthread      - szálak létrehozása és kezelése
 *  5. condition variable - szálak közti szinkronizáció (producer-consumer)
 *  6. signal       - SIGINT (Ctrl+C) kezelése, kulturált leállítás
 *  7. cleanup      - erőforrások felszabadítása, szálak/processzek bevárása
 * 
 *  A program működése:
 *  -------------------
 *  
 *     [SZENZOR PROCESS]              [MAIN PROCESS]
 *           |                              |
 *           |   pipe (egyirányú cső)       |
 *           | ----------------------------> |
 *           |   "TEMP 23C"                 |
 *           |   "TEMP 25C"                 |
 *                                          |
 *                                    +-----+-----+
 *                                    |           |
 *                              [MAIN LOOP]  [WORKER THREAD]
 *                              (olvas pipe)  (feldolgoz)
 *                                    |           |
 *                                    +--(cond)---+
 *                                    producer    consumer
 * 
 *  Leállítás Ctrl+C-vel:
 *  ---------------------
 *  1. SIGINT handler beállítja: running = 0
 *  2. Main loop kilép
 *  3. Gyerek process-nek SIGTERM-et küldünk
 *  4. Worker thread-et felébresztjük (broadcast)
 *  5. Mindent bevárunk és cleanup
 * 
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* ============================================================================
 *  KONSTANSOK
 * ============================================================================ */

#define LINE_MAX_LEN 64     /* Maximum sor hossz a pipe-ban */

/* ============================================================================
 *  GLOBÁLIS VÁLTOZÓK (szálak közötti megosztás)
 * ============================================================================ */

/* Mutex és condition variable a producer-consumer mintához */
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;

/* A legutóbbi beolvasott sor (megosztott puffer) */
static char latest_line[LINE_MAX_LEN];

/* Van-e feldolgozandó adat? (1 = igen, 0 = nem) */
static int has_data = 0;

/* Futás flag - volatile mert signal handler módosítja */
static volatile sig_atomic_t running = 1;

/* Gyerek process PID-je (leállításhoz kell) */
static pid_t child_pid = -1;

/* ============================================================================
 *  SEGÉDFÜGGVÉNYEK
 * ============================================================================ */

/**
 * Hiba esetén kiírja az üzenetet és kilép.
 */
static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/**
 * SIGINT (Ctrl+C) kezelő.
 * 
 * FONTOS: Signal handler-ben csak async-signal-safe függvényeket szabad
 * használni! (ezért csak egy flag-et állítunk be)
 */
static void sigint_handler(int sig)
{
    (void)sig;  /* Nem használjuk, de el kell fogadni a paramétert */
    running = 0;
}

/**
 * Véletlenszerű hőmérséklet generálása (18-32 fok között).
 */
static int random_temperature(void)
{
    return 18 + (rand() % 15);
}

/* ============================================================================
 *  SZENZOR PROCESS (GYEREK)
 * 
 *  Ez a függvény a fork() után a gyerek process-ben fut.
 *  Feladata: szimulált hőmérséklet adatokat írni a pipe-ba.
 * ============================================================================ */

static void sensor_process(int write_fd)
{
    /* Inicializáljuk a random generátort egyedi seed-del */
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    
    printf("[SZENZOR] Elindult (PID: %d)\n", getpid());
    fflush(stdout);
    
    while (1) {
        char buffer[LINE_MAX_LEN];
        int temp = random_temperature();
        
        /* Formázzuk az üzenetet */
        int len = snprintf(buffer, sizeof(buffer), "TEMP %dC\n", temp);
        if (len < 0) {
            _exit(EXIT_FAILURE);
        }
        
        /* Írunk a pipe-ba */
        ssize_t written = write(write_fd, buffer, (size_t)len);
        if (written < 0) {
            /* Ha a pipe lezárult (SIGPIPE), kilépünk */
            _exit(EXIT_SUCCESS);
        }
        
        /* Várunk 1 másodpercet a következő mérés előtt */
        sleep(1);
    }
}

/* ============================================================================
 *  WORKER THREAD (FOGYASZTÓ / CONSUMER)
 * 
 *  Ez a szál feldolgozza a beolvasott adatokat.
 *  Producer-consumer minta: vár az új adatra condition variable-lel.
 * ============================================================================ */

static void *worker_thread(void *arg)
{
    (void)arg;  /* Nem használunk paramétert */
    
    printf("[WORKER] Szál elindult\n");
    fflush(stdout);
    
    while (1) {
        char line[LINE_MAX_LEN];
        
        /* === KRITIKUS SZAKASZ KEZDETE === */
        pthread_mutex_lock(&mtx);
        
        /* Várakozás amíg van adat VAGY le kell állni
         * 
         * FONTOS: while ciklus kell, mert:
         *   1. Spurious wakeup: a thread véletlenül is felébredhet
         *   2. Többszörös consumer esetén másik thread elviheti az adatot
         */
        while (running && !has_data) {
            /* 
             * pthread_cond_wait atomikusan:
             *   1. Elengedi a mutex-et
             *   2. Elaltatja a szálat
             *   3. Felébredéskor visszaveszi a mutex-et
             */
            pthread_cond_wait(&cond, &mtx);
        }
        
        /* Ha le kell állni ÉS nincs több adat, kilépünk */
        if (!running && !has_data) {
            pthread_mutex_unlock(&mtx);
            break;
        }
        
        /* Kimásoljuk az adatot lokális változóba */
        strncpy(line, latest_line, sizeof(line));
        line[sizeof(line) - 1] = '\0';
        has_data = 0;  /* Jelezzük, hogy elvittük az adatot */
        
        pthread_mutex_unlock(&mtx);
        /* === KRITIKUS SZAKASZ VÉGE === */
        
        /* Feldolgozás (mutex NÉLKÜL, hogy ne blokkoljuk a producert) */
        printf("[WORKER] Feldolgozva: %s", line);
        fflush(stdout);
    }
    
    printf("[WORKER] Szál leállt\n");
    fflush(stdout);
    
    return NULL;
}

/* ============================================================================
 *  MAIN FÜGGVÉNY
 * ============================================================================ */

int main(void)
{
    printf("========================================\n");
    printf("  LAB 05 - Pipe + Thread + Signal\n");
    printf("  Leállítás: Ctrl+C\n");
    printf("========================================\n\n");
    
    /* 
     * 1. LÉPÉS: SIGINT kezelő beállítása
     * ----------------------------------
     * Ctrl+C lenyomásakor a sigint_handler fut le.
     */
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        die("signal() hiba");
    }
    
    /* 
     * 2. LÉPÉS: Pipe létrehozása
     * --------------------------
     * pipe_fd[0] = olvasó vég (a szülő használja)
     * pipe_fd[1] = író vég (a gyerek használja)
     * 
     *   [Gyerek] ---write---> pipe_fd[1] ====PIPE==== pipe_fd[0] ---read---> [Szülő]
     */
    int pipe_fd[2];
    if (pipe(pipe_fd) < 0) {
        die("pipe() hiba");
    }
    
    printf("[MAIN] Pipe létrehozva (olvasó: fd=%d, író: fd=%d)\n", 
           pipe_fd[0], pipe_fd[1]);
    
    /* 
     * 3. LÉPÉS: Gyerek process létrehozása
     * ------------------------------------
     * fork() után:
     *   - Gyerek process-ben: child_pid = 0
     *   - Szülő process-ben: child_pid = gyerek PID-je
     */
    child_pid = fork();
    if (child_pid < 0) {
        die("fork() hiba");
    }
    
    if (child_pid == 0) {
        /* === GYEREK PROCESS === */
        
        /* A gyereknek nem kell az olvasó vég */
        close(pipe_fd[0]);
        
        /* Futtatjuk a szenzor logikát */
        sensor_process(pipe_fd[1]);
        
        /* Ide soha nem jutunk (sensor_process végtelen ciklus) */
        _exit(EXIT_SUCCESS);
    }
    
    /* === SZÜLŐ PROCESS (innentől) === */
    
    printf("[MAIN] Gyerek process elindítva (PID: %d)\n", child_pid);
    
    /* A szülőnek nem kell az író vég */
    close(pipe_fd[1]);
    
    /* 
     * 4. LÉPÉS: Worker thread létrehozása
     * -----------------------------------
     */
    pthread_t worker;
    if (pthread_create(&worker, NULL, worker_thread, NULL) != 0) {
        die("pthread_create() hiba");
    }
    
    printf("[MAIN] Várakozás szenzor adatokra...\n\n");
    
    /* 
     * 5. LÉPÉS: Fő ciklus (PRODUCER)
     * ------------------------------
     * Olvassuk a pipe-ból karakterenként és sorokat állítunk össze.
     */
    char line[LINE_MAX_LEN];
    size_t idx = 0;
    
    while (running) {
        char ch;
        ssize_t bytes_read = read(pipe_fd[0], &ch, 1);
        
        if (bytes_read < 0) {
            /* Ha megszakítás volt (EINTR), próbáljuk újra */
            if (errno == EINTR) {
                continue;
            }
            die("read() hiba");
        }
        
        if (bytes_read == 0) {
            /* Pipe lezárult (EOF) */
            printf("[MAIN] Pipe lezárult\n");
            break;
        }
        
        /* Karakter hozzáadása a sorhoz */
        if (idx + 1 < sizeof(line)) {
            line[idx++] = ch;
        }
        
        /* Ha sor vége, átadjuk a workernek */
        if (ch == '\n') {
            line[idx] = '\0';
            idx = 0;
            
            printf("[MAIN] Beolvasva: %s", line);
            
            /* === KRITIKUS SZAKASZ: átadjuk az adatot === */
            pthread_mutex_lock(&mtx);
            strncpy(latest_line, line, sizeof(latest_line));
            latest_line[sizeof(latest_line) - 1] = '\0';
            has_data = 1;
            pthread_cond_signal(&cond);  /* Felébresztjük a workert */
            pthread_mutex_unlock(&mtx);
        }
    }
    
    /* 
     * 6. LÉPÉS: Kulturált leállítás (CLEANUP)
     * ---------------------------------------
     */
    printf("\n[MAIN] Leállítás megkezdése...\n");
    
    /* 6a. Gyerek process leállítása */
    if (child_pid > 0) {
        printf("[MAIN] Gyerek process leállítása (SIGTERM)...\n");
        kill(child_pid, SIGTERM);
        waitpid(child_pid, NULL, 0);  /* Bevárjuk a gyereket */
        printf("[MAIN] Gyerek process leállt\n");
    }
    
    /* 6b. Worker thread leállítása */
    pthread_mutex_lock(&mtx);
    running = 0;
    pthread_cond_broadcast(&cond);  /* Minden várakozó szálat felébresztünk */
    pthread_mutex_unlock(&mtx);
    
    printf("[MAIN] Worker thread bevárása...\n");
    pthread_join(worker, NULL);
    
    /* 6c. Erőforrások felszabadítása */
    close(pipe_fd[0]);
    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&cond);
    
    printf("\n========================================\n");
    printf("  LEÁLLÍTÁS SIKERES!\n");
    printf("========================================\n");
    
    return EXIT_SUCCESS;
}
