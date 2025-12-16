#include "semafori.h"
#include "config.h"
#include "utilita.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static const char* SEM_KEY_PATH = "/tmp/poste_sem.key";

static key_t get_sem_key(void) {
    int fd = open(SEM_KEY_PATH, O_RDONLY | O_CREAT, 0666);
    if (fd == -1) TEST_ERROR(-1, "open key file semafori");
    close(fd);

    key_t k = ftok(SEM_KEY_PATH, 'S');
    if (k == (key_t)-1) TEST_ERROR(-1, "ftok semafori");
    
    return k;
}

static int total_semaphores(void) {
    return SEM_SOSTITUZIONE_BASE + NUM_SERVIZI;
}

static void init_all_values(int semid, int n) {
    if (semctl(semid, SEM_MUTEX, SETVAL, 1) == -1) 
        TEST_ERROR(-1, "semctl SETVAL MUTEX");

    for (int i = 0; i < n; ++i) {
        if (i == SEM_MUTEX) continue;
        
        if (semctl(semid, i, SETVAL, 0) == -1) 
            TEST_ERROR(-1, "semctl SETVAL 0");
    }
}

int inizializza_semafori(void) {
    key_t k = get_sem_key();
    int n = total_semaphores();

    int semid = semget(k, n, IPC_CREAT | IPC_EXCL | 0666);

    if (semid != -1) {
        init_all_values(semid, n);
    } else {
        if (errno == EEXIST) {
            semid = semget(k, n, 0666);
            if (semid == -1) TEST_ERROR(-1, "semget esistente");
        } else {
            TEST_ERROR(-1, "semget creation");
        }
    }

    return semid;
}

int ottieni_semafori(void) {
    key_t k = get_sem_key();
    int semid = semget(k, total_semaphores(), 0666);
    
    if (semid == -1 && errno != ENOENT) {
        perror("semget ottieni");
    }
    return semid;
}

void rimuovi_semafori(int semid) {
    if (semctl(semid, 0, IPC_RMID, 0) == -1) {
        if (errno != EIDRM && errno != EINVAL) {
            perror("semctl IPC_RMID");
        }
    }
}

int sem_wait_operation(int semid, int semnum, int count) {
    struct sembuf op;
    op.sem_num = (unsigned short)semnum;
    op.sem_op = (short)-count;
    op.sem_flg = 0;

    while (1) {
        if (semop(semid, &op, 1) == 0) return 0;
        if (errno == EINTR) continue;
        return -1;
    }
}

int sem_post_operation(int semid, int semnum, int count) {
    struct sembuf op;
    op.sem_num = (unsigned short)semnum;
    op.sem_op = (short)count;
    op.sem_flg = 0;

    while (1) {
        if (semop(semid, &op, 1) == 0) return 0;
        if (errno == EINTR) continue;
        return -1;
    }
}