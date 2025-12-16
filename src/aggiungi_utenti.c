#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h> 
#include <signal.h>   
#include <errno.h>
#include <time.h>

#include "config.h"
#include "memoria_condivisa.h"
#include "semafori.h"
#include "utilita.h"         

#ifndef MAX_UTENTI_DINAMICI
#define MAX_UTENTI_DINAMICI 100 
#endif

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <numero_nuovi_utenti> [--config=percorso]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int nuovi_utenti = atoi(argv[1]);
    if (nuovi_utenti <= 0) {
        fprintf(stderr, "Errore: numero utenti non valido.\n");
        exit(EXIT_FAILURE);
    }

    int shmid = prova_ottieni_memoria_condivisa();
    
    if (shmid < 0) {
        usleep(600000); 
        shmid = prova_ottieni_memoria_condivisa();
    }
    
    if (shmid < 0) {
        fprintf(stderr, "\n[Aggiungi Utenti] ERRORE: Simulazione non trovata dopo 2 tentativi.\n");
        fprintf(stderr, "Assicurati che 'make run' sia attivo e in esecuzione.\n");
        exit(EXIT_FAILURE);
    }
    
    printf("[Aggiungi Utenti] Simulazione trovata! Aggiungo %d utenti...\n", nuovi_utenti);

    srand((unsigned)(time(NULL) ^ getpid()));

    SharedMemory* shm = (SharedMemory*)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) TEST_ERROR(-1, "shmat fallita");

    int sem_id = ottieni_semafori(); 
    if (sem_id < 0) TEST_ERROR(-1, "semget semafori fallita");
    
    int dummy; float f_dummy;
    const char* cfg = risolvi_percorso_config(argc, argv);
    carica_configurazione(cfg, &dummy, &dummy, &dummy, &dummy, &dummy, 
                          &dummy, &f_dummy, &f_dummy, &dummy, &dummy, &dummy);


    for (int i = 0; i < nuovi_utenti; i++) {
        
        sem_wait_operation(sem_id, SEM_MUTEX, 1);
        
        if (shm->num_utenti_dinamici >= MAX_UTENTI_DINAMICI) {
            sem_post_operation(sem_id, SEM_MUTEX, 1);
            fprintf(stderr, "[Generatore] Limite massimo utenti raggiunto (%d). Interruzione.\n", MAX_UTENTI_DINAMICI);
            break;
        }

        shm->num_processi_utente_totali++;
        int id_numerico = shm->num_processi_utente_totali;
        
        sem_post_operation(sem_id, SEM_MUTEX, 1);

        pid_t pid = fork();
        TEST_ERROR(pid, "fork fallita");

        if (pid == 0) {
            char id_str[16];
            snprintf(id_str, sizeof(id_str), "%d", id_numerico);
            
            char *args[] = {UTENTE_EXEC, id_str, NULL};
            execv(UTENTE_EXEC, args);
            
            TEST_ERROR(-1, "execv fallita"); 
        } 
        else {
            sem_wait_operation(sem_id, SEM_MUTEX, 1);
            shm->utenti_dinamici[shm->num_utenti_dinamici] = pid;
            shm->num_utenti_dinamici++;
            sem_post_operation(sem_id, SEM_MUTEX, 1);
            
            printf("[Aggiungi Utenti] Creato utente %d (PID %d)\n", id_numerico, pid);
        }
    }
    
    if (shmdt(shm) == -1) perror("shmdt");

    return 0;
}