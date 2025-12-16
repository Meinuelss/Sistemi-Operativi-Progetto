#include "memoria_condivisa.h"
#include "utilita.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

static key_t get_shm_key(void) {
    key_t k = ftok(FTOK_PATH, 'S');
    if (k == (key_t)-1) TEST_ERROR(-1, "ftok shm");
    return k;
}

int inizializza_memoria_condivisa(void) {
    key_t k = get_shm_key();
    
    int shmid = shmget(k, sizeof(SharedMemory), IPC_CREAT | IPC_EXCL | 0666);
    
    if (shmid == -1) {
        if (errno == EEXIST) {
            shmid = shmget(k, sizeof(SharedMemory), 0666);
            if (shmid == -1) TEST_ERROR(-1, "shmget existing");
        } else {
            TEST_ERROR(-1, "shmget create");
        }
    } else {
        SharedMemory* shm = (SharedMemory*)shmat(shmid, NULL, 0);
        if (shm == (void*)-1) TEST_ERROR(-1, "shmat init");
        
        memset(shm, 0, sizeof(SharedMemory));
        
        shm->attivo = 1;
        shm->giorno = 1;
        
        shmdt(shm);
    }
    
    return shmid;
}

int ottieni_memoria_condivisa(void) {
    key_t k = get_shm_key();
    int shmid = shmget(k, sizeof(SharedMemory), 0666);
    if (shmid == -1) TEST_ERROR(-1, "shmget obtain");
    return shmid;
}

int prova_ottieni_memoria_condivisa(void) {
    key_t k = get_shm_key();
    return shmget(k, sizeof(SharedMemory), 0666);
}

SharedMemory* collega_memoria_condivisa(int shmid) {
    void* p = shmat(shmid, NULL, 0);
    if (p == (void*)-1) return NULL;
    return (SharedMemory*)p;
}

void scollega_memoria_condivisa(SharedMemory* shm) {
    if (shmdt(shm) == -1) {
        if (errno != EINVAL) perror("shmdt");
    }
}

void rimuovi_memoria_condivisa(int shmid) {
    shmctl(shmid, IPC_RMID, NULL);
}