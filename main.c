/**
 * ============================================================================
 *  LAB 05 - Operacios Rendszerek Labor
 * ============================================================================
 * 
 *  TEMA: Pipe + Thread + Signal kezeles
 * 
 *  FELADAT: Toltsd ki a TODO reszeket!
 * 
 *  A program mukodese:
 *  -------------------
 *  - Gyerek process (szenzor): homerseklet adatokat ir a pipe-ba
 *  - Szulo process: olvassa a pipe-ot, atadja a worker thread-nek
 *  - Worker thread: feldolgozza az adatokat (condition variable-lel var)
 *  - Ctrl+C: kulturalt leallitas
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

#define LINE_MAX_LEN 64

/* Mutex es condition variable a producer-consumer mintahoz */
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;

/* A legutobbi beolvasott sor (megosztott puffer) */
static char latest_line[LINE_MAX_LEN];

/* Van-e feldolgozando adat? (1 = igen, 0 = nem) */
static int has_data = 0;

/* Futas flag - volatile mert signal handler modositja */
static volatile sig_atomic_t running = 1;

/* Gyerek process PID-je */
static pid_t child_pid = -1;

/* Segedfuggveny: hiba eseten kiir es kilep */
static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/**
 * TODO 1: SIGINT kezelo implementalasa
 * 
 * Cel: Ctrl+C lenyomasakor allitsd be a running valtozot 0-ra.
 * 
 * TIPP: A signal handler-ben csak a running flaget szabad modositani,
 *       mert csak "async-signal-safe" fuggvenyeket lehet hasznalni.
 */
static void sigint_handler(int sig)
{
    (void)sig;
    /* TODO: Allitsd running-ot 0-ra */

}

/* Veletlen homerseklet generalasa (18-32 fok kozott) */
static int random_temperature(void)
{
    return 18 + (rand() % 15);
}

/**
 * SZENZOR PROCESS (gyerek)
 * 
 * Ez a fuggveny a fork() utan a gyerek process-ben fut.
 * Masodpercenkent ir a pipe-ba egy homerseklet erteket.
 */
static void sensor_process(int write_fd)
{
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    
    printf("[SZENZOR] Elindult (PID: %d)\n", getpid());
    fflush(stdout);
    
    while (1) {
        char buffer[LINE_MAX_LEN];
        int temp = random_temperature();
        
        int len = snprintf(buffer, sizeof(buffer), "TEMP %dC\n", temp);
        if (len < 0) {
            _exit(EXIT_FAILURE);
        }
        
        /**
         * TODO 2: Iras a pipe-ba
         * 
         * Cel: Ird ki a buffer tartalmat a pipe-ba.
         * 
         * Hasznald: write(write_fd, buffer, (size_t)len)
         * Ha write() < 0, hivd meg: _exit(EXIT_SUCCESS)
         */
        /* TODO: Implementald a write() hivast */


        sleep(1);
    }
}

/**
 * WORKER THREAD (fogyaszto / consumer)
 * 
 * Ez a szal feldolgozza a beolvasott adatokat.
 * Condition variable-lel var az uj adatra.
 */
static void *worker_thread(void *arg)
{
    (void)arg;
    
    printf("[WORKER] Szal elindult\n");
    fflush(stdout);
    
    while (1) {
        char line[LINE_MAX_LEN];
        
        pthread_mutex_lock(&mtx);
        
        /**
         * TODO 3: Varakozas condition variable-lel
         * 
         * Cel: Varj amig van adat (has_data == 1) VAGY le kell allni (!running)
         * 
         * Hasznald: pthread_cond_wait(&cond, &mtx)
         * 
         * FONTOS: while ciklusban kell varni, nem if-fel!
         *         Feltetel: running && !has_data
         */
        /* TODO: Implementald a while ciklust pthread_cond_wait-tel */


        /* Ha le kell allni ES nincs tobb adat, kilepunk */
        if (!running && !has_data) {
            pthread_mutex_unlock(&mtx);
            break;
        }
        
        /* Kimasoljuk az adatot lokalis valtozoba */
        strncpy(line, latest_line, sizeof(line));
        line[sizeof(line) - 1] = '\0';
        has_data = 0;
        
        pthread_mutex_unlock(&mtx);
        
        printf("[WORKER] Feldolgozva: %s", line);
        fflush(stdout);
    }
    
    printf("[WORKER] Szal leallt\n");
    fflush(stdout);
    
    return NULL;
}

int main(void)
{
    printf("========================================\n");
    printf("  LAB 05 - Pipe + Thread + Signal\n");
    printf("  Leallitas: Ctrl+C\n");
    printf("========================================\n\n");
    
    /**
     * TODO 4: SIGINT kezelo beallitasa
     * 
     * Cel: Ctrl+C lenyomasakor a sigint_handler fusson le.
     * 
     * Hasznald: signal(SIGINT, sigint_handler)
     * Ha SIG_ERR-t ad vissza, hivd meg: die("signal() hiba")
     */
    /* TODO: Allitsd be a SIGINT kezelot */


    /**
     * TODO 5: Pipe letrehozasa
     * 
     * Cel: Hozz letre egy pipe-ot a processzek kozti kommunikaciora.
     * 
     * Hasznald: pipe(pipe_fd)
     * pipe_fd[0] = olvaso veg (szulo hasznalja)
     * pipe_fd[1] = iro veg (gyerek hasznalja)
     * 
     * Ha hiba, hivd meg: die("pipe() hiba")
     */
    int pipe_fd[2];
    /* TODO: Hozd letre a pipe-ot */


    printf("[MAIN] Pipe letrehozva\n");
    
    /**
     * TODO 6: Gyerek process letrehozasa fork()-kal
     * 
     * Cel: Hozz letre egy gyerek process-t.
     * 
     * Hasznald: fork()
     * Visszateresi ertek:
     *   - < 0: hiba (hivd meg: die("fork() hiba"))
     *   - == 0: gyerek process
     *   - > 0: szulo process (child_pid = gyerek PID-je)
     */
    /* TODO: Hivd meg a fork()-ot es mentsd child_pid-be */


    if (child_pid == 0) {
        /* GYEREK PROCESS */
        
        /**
         * TODO 7: Zard be a gyereknek nem kello pipe veget
         * 
         * Cel: A gyereknek csak az iro veg kell (pipe_fd[1]).
         *      Zard be az olvaso veget (pipe_fd[0]).
         */
        /* TODO: Zard be pipe_fd[0]-t */


        sensor_process(pipe_fd[1]);
        _exit(EXIT_SUCCESS);
    }
    
    /* SZULO PROCESS (innentol) */
    
    printf("[MAIN] Gyerek process elindult (PID: %d)\n", child_pid);
    
    /**
     * TODO 8: Zard be a szulonek nem kello pipe veget
     * 
     * Cel: A szulonek csak az olvaso veg kell (pipe_fd[0]).
     *      Zard be az iro veget (pipe_fd[1]).
     */
    /* TODO: Zard be pipe_fd[1]-et */


    /* Worker thread letrehozasa */
    pthread_t worker;
    if (pthread_create(&worker, NULL, worker_thread, NULL) != 0) {
        die("pthread_create() hiba");
    }
    
    printf("[MAIN] Varakozas szenzor adatokra...\n\n");
    
    /* Fo ciklus: olvassuk a pipe-ot es atadjuk a workernek */
    char line[LINE_MAX_LEN];
    size_t idx = 0;
    
    while (running) {
        char ch;
        ssize_t bytes_read = read(pipe_fd[0], &ch, 1);
        
        if (bytes_read < 0) {
            if (errno == EINTR) continue;
            die("read() hiba");
        }
        
        if (bytes_read == 0) {
            printf("[MAIN] Pipe lezarult\n");
            break;
        }
        
        if (idx + 1 < sizeof(line)) {
            line[idx++] = ch;
        }
        
        if (ch == '\n') {
            line[idx] = '\0';
            idx = 0;
            
            printf("[MAIN] Beolvasva: %s", line);
            
            /**
             * TODO 9: Adat atadasa a worker thread-nek
             * 
             * Cel: Masold be a line-t latest_line-ba,
             *      allitsd has_data-t 1-re,
             *      es ebreszd fel a workert.
             * 
             * Lepesek:
             *   1. pthread_mutex_lock(&mtx)
             *   2. strncpy(latest_line, line, sizeof(latest_line))
             *   3. has_data = 1
             *   4. pthread_cond_signal(&cond)
             *   5. pthread_mutex_unlock(&mtx)
             */
            /* TODO: Implementald az adat atadasat */


        }
    }
    
    /* KULTURALT LEALLITAS */
    
    printf("\n[MAIN] Leallitas megkezdese...\n");
    
    /**
     * TODO 10: Gyerek process leallitasa
     * 
     * Cel: Kuldd el a SIGTERM signalt a gyereknek, majd vard be.
     * 
     * Hasznald:
     *   1. kill(child_pid, SIGTERM)
     *   2. waitpid(child_pid, NULL, 0)
     */
    if (child_pid > 0) {
        printf("[MAIN] Gyerek process leallitasa...\n");
        /* TODO: Allitsd le es vard be a gyereket */


        printf("[MAIN] Gyerek process leallt\n");
    }
    
    /**
     * TODO 11: Worker thread leallitasa
     * 
     * Cel: Ebreszd fel a workert, hogy ki tudjon lepni.
     * 
     * Lepesek:
     *   1. pthread_mutex_lock(&mtx)
     *   2. running = 0
     *   3. pthread_cond_broadcast(&cond)  <- FONTOS: broadcast, nem signal!
     *   4. pthread_mutex_unlock(&mtx)
     */
    /* TODO: Ebreszd fel a workert */


    printf("[MAIN] Worker thread bevarasa...\n");
    pthread_join(worker, NULL);
    
    /* Eroforrasok felszabaditasa */
    close(pipe_fd[0]);
    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&cond);
    
    printf("\n========================================\n");
    printf("  LEALLITAS SIKERES!\n");
    printf("========================================\n");
    
    return EXIT_SUCCESS;
}
