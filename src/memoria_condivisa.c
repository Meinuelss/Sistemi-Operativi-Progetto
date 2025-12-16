#include "memoria_condivisa.h"
#include "coda.h"
#include "semafori.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>

static key_t get_shm_key(void) {
    int fd = open(FTOK_PATH, O_RDONLY | O_CREAT, 0666);
    if (fd == -1) { perror("open(shared_memory key file)"); exit(1); }
    close(fd);
    key_t k = ftok(FTOK_PATH, 'S');
    if (k == (key_t)-1) { perror("ftok(shared_memory)"); exit(1); }
    return k;
}

static key_t try_get_shm_key(void) {
    const char *percorso = "/tmp/poste_shm.key";
    if (access(percorso, F_OK) != 0) {
        return (key_t)-1;
    }
    key_t k = ftok(percorso, 'S');
    if (k == (key_t)-1) {
        perror("ftok(shared_memory)");
        return (key_t)-1;
    }
    return k;
}

int inizializza_memoria_condivisa() {
    key_t k = get_shm_key();
    size_t size = sizeof(SharedMemory);
    int shmid = shmget(k, size, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1) { 
        fprintf(stderr, "[ERROR] shmget fallito: key=0x%x size=%zu errno=%d\n", k, size, errno);
        perror("shmget"); 
        exit(1); 
    }

    SharedMemory* shm = (SharedMemory*)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) { perror("shmat"); exit(1); }

    memset(shm, 0, sizeof(SharedMemory));

    for (int i = 0; i < NUM_SERVIZI; i++) {
        inizializza_coda(&shm->code_servizio[i]);
    }

    shm->giorno = 1;
    shm->attivo = 1;

    for (int i = 0; i < MAX_SPORTELLI; ++i) {
        shm->sportelli[i].tipo_servizio = 1;
    }

    if (shmdt(shm) == -1) { perror("shmdt"); exit(1); }
    return shmid;
}

int ottieni_memoria_condivisa() {
    key_t k = get_shm_key();
    int shmid = shmget(k, sizeof(SharedMemory), 0666);
    if (shmid == -1) { perror("shmget (get)"); exit(1); }
    return shmid;
}

int prova_ottieni_memoria_condivisa(void) {
    key_t k = try_get_shm_key();
    if (k == (key_t)-1) {
        return -1;
    }
    int shmid = shmget(k, sizeof(SharedMemory), 0666);
    if (shmid == -1) {
        return -1;
    }
    return shmid;
}

SharedMemory* collega_memoria_condivisa(int shmid) {
    SharedMemory* shm = (SharedMemory*)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) { perror("shmat"); exit(1); }
    return shm;
}

void scollega_memoria_condivisa(SharedMemory* shm) {
    if (shmdt(shm) == -1) { perror("shmdt"); exit(1); }
}

void rimuovi_memoria_condivisa(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) { perror("shmctl"); exit(1); }
}
