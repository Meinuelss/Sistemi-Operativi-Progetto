#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <errno.h>

#include "memoria_condivisa.h"
#include "semafori.h"
#include "messaggi.h"
#include "coda.h"
#include "config.h"
#include "utilita.h"

static volatile sig_atomic_t termination_requested = 0;

static void signal_handler(int sig) {
    (void)sig;
    termination_requested = 1;
}

int main(void) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    TEST_ERROR(sigaction(SIGTERM, &sa, NULL), "sigaction SIGTERM");
    TEST_ERROR(sigaction(SIGINT, &sa, NULL), "sigaction SIGINT");

    int shmid = ottieni_memoria_condivisa();
    if (shmid < 0) TEST_ERROR(-1, "shmget erogatore");

    SharedMemory* shm = collega_memoria_condivisa(shmid);
    if (!shm) TEST_ERROR(-1, "shmat erogatore");

    int sem_id = ottieni_semafori();
    if (sem_id < 0) TEST_ERROR(-1, "semget erogatore");

    int msgid  = shm->msgqueue_id;

    sem_wait_operation(sem_id, SEM_MUTEX, 1);
    shm->erogatore_inizializzato = 1;
    sem_post_operation(sem_id, SEM_MUTEX, 1);

    printf("[Erogatore] Avviato (PID %d). In attesa di richieste...\n", getpid());
    
    sem_post_operation(sem_id, SEM_INIT_BARRIER, 1);

    while (shm->attivo && !termination_requested) {
        MessaggioRichiesta richiesta;
        
        ssize_t n = msgrcv(msgid, &richiesta, sizeof(richiesta) - sizeof(long),
                           TIPO_MSG_RICHIESTA, 0);
        
        if (n == -1) {
            if (errno == EINTR) {
                if (termination_requested || !shm->attivo) break;
                continue;
            }
            if (errno == EIDRM || errno == EINVAL) {
                break;
            }
            perror("[Erogatore] msgrcv error");
            break;
        }

        int servizio = richiesta.tipo_servizio;
        
        if (servizio < 1 || servizio > NUM_SERVIZI) {
            fprintf(stderr, "[Erogatore] Warning: Servizio %d non valido (PID=%d)\n", 
                    servizio, (int)richiesta.pid_utente);
            continue;
        }
        
        int indice = servizio - 1;
        pid_t pid_u = (pid_t)richiesta.pid_utente;
        
        int giorno_att, aperto, min_att, sportelli_disp;
        
        sem_wait_operation(sem_id, SEM_MUTEX, 1);
        giorno_att     = shm->giorno;
        aperto         = shm->giorno_aperto;
        min_att        = shm->minuto_corrente;
        sportelli_disp = shm->sportelli_per_servizio_oggi[indice];
        sem_post_operation(sem_id, SEM_MUTEX, 1);

        if (!aperto) {
            invia_risposta(msgid, pid_u, ESITO_NON_SERVITO_GIORNO, servizio, giorno_att);
            continue;
        }
        
        if (sportelli_disp == 0) {            
            sem_wait_operation(sem_id, SEM_MUTEX, 1);
            shm->servizi_non_completati_oggi[indice]++;
            shm->totale_servizi_non_completati[indice]++;
            sem_post_operation(sem_id, SEM_MUTEX, 1);
            
            invia_risposta(msgid, pid_u, ESITO_NON_SERVITO_GIORNO, servizio, giorno_att);
            continue;
        }

        ElementoCoda e;
        e.pid = pid_u;
        e.giorno = giorno_att;
        e.minuto = min_att;

        sem_wait_operation(sem_id, SEM_MUTEX, 1);
        int ok = accoda(&shm->code_servizio[indice], e);
        if (ok == 0) {
            shm->utenti_in_attesa++;
        }
        sem_post_operation(sem_id, SEM_MUTEX, 1);

        if (ok == 0) {
            sem_post_operation(sem_id, SEM_BASE_SERVIZI + indice, 1);
        } else {
            fprintf(stderr, "[Erogatore] Coda S%d PIENA! PID %d respinto.\n", servizio, pid_u);
            
            sem_wait_operation(sem_id, SEM_MUTEX, 1);
            shm->servizi_non_completati_oggi[indice]++;
            shm->totale_servizi_non_completati[indice]++;
            sem_post_operation(sem_id, SEM_MUTEX, 1);
            
            invia_risposta(msgid, pid_u, ESITO_NON_SERVITO_GIORNO, servizio, giorno_att);
        }
    }

    scollega_memoria_condivisa(shm);
    return 0;
}