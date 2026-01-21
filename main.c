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

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

static char latest_line[LINE_MAX_LEN];
static int has_item = 0;

static volatile sig_atomic_t running = 1;
static pid_t child_pid = -1;

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

static int rand_price(void) {
    return 100 + (rand() % 200);
}

static const char *rand_stock(void) {
    const char *stocks[] = {"AAPL", "GOOG", "MSFT", "TSLA"};
    return stocks[rand() % 4];
}

static void *worker_fn(void *arg) {
    (void)arg;

    while (1) {
        char line[LINE_MAX_LEN];

        pthread_mutex_lock(&mtx);
        while (running && !has_item) {
            pthread_cond_wait(&cv, &mtx);
        }

        if (!running && !has_item) {
            pthread_mutex_unlock(&mtx);
            break;
        }

        strncpy(line, latest_line, sizeof(line));
        line[sizeof(line) - 1] = '\0';
        has_item = 0;

        pthread_mutex_unlock(&mtx);

        printf("[WORKER] got: %s", line);
        fflush(stdout);
    }

    return NULL;
}

int main(void) {
    srand((unsigned)time(NULL));

    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        die("signal");
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) < 0) {
        die("pipe");
    }

    child_pid = fork();
    if (child_pid < 0) {
        die("fork");
    }

    if (child_pid == 0) {
        close(pipe_fd[0]);

        while (1) {
            char buf[LINE_MAX_LEN];
            int price = rand_price();
            const char *stock = rand_stock();

            int n = snprintf(buf, sizeof(buf), "%s %d\n", stock, price);
            if (n < 0) _exit(1);

            ssize_t w = write(pipe_fd[1], buf, (size_t)n);
            if (w < 0) {
                _exit(0);
            }

            sleep(1);
        }

        _exit(0);
    }

    close(pipe_fd[1]);

    pthread_t worker;
    if (pthread_create(&worker, NULL, worker_fn, NULL) != 0) {
        die("pthread_create");
    }

    char line[LINE_MAX_LEN];
    size_t idx = 0;

    while (running) {
        char ch;
        ssize_t r = read(pipe_fd[0], &ch, 1);
        if (r < 0) {
            if (errno == EINTR) continue;
            die("read");
        }
        if (r == 0) {
            break;
        }

        if (idx + 1 < sizeof(line)) {
            line[idx++] = ch;
        }

        if (ch == '\n') {
            line[idx] = '\0';
            idx = 0;

            pthread_mutex_lock(&mtx);
            strncpy(latest_line, line, sizeof(latest_line));
            latest_line[sizeof(latest_line) - 1] = '\0';
            has_item = 1;
            pthread_cond_signal(&cv);
            pthread_mutex_unlock(&mtx);
        }
    }

    if (child_pid > 0) {
        kill(child_pid, SIGTERM);
        waitpid(child_pid, NULL, 0);
    }

    pthread_mutex_lock(&mtx);
    running = 0;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&mtx);

    pthread_join(worker, NULL);

    close(pipe_fd[0]);

    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&cv);

    printf("\n[MAIN] shutdown complete.\n");
    return 0;
}