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
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    int shmid = ottieni_memoria_condivisa();
    SharedMemory* shm = collega_memoria_condivisa(shmid);
    int sem_id = ottieni_semafori();

    int msgid  = shm->msgqueue_id;

    sem_wait_operation(sem_id, SEM_MUTEX, 1);
    shm->erogatore_inizializzato = 1;
    sem_post_operation(sem_id, SEM_MUTEX, 1);

    printf("Erogatore ticket avviato. In attesa di richieste...\n");
    
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
            if (!shm->attivo || termination_requested) break;
            perror("erogatore_ticket msgrcv");
            continue;
        }

        int servizio = richiesta.tipo_servizio;
        if (servizio < 1 || servizio > NUM_SERVIZI) {
            fprintf(stderr, "[Erogatore] Servizio %d non valido (PID=%d)\n", servizio, (int)richiesta.pid_utente);
            continue;
        }
        int indice = servizio - 1;
        int pid_u = (int)richiesta.pid_utente;
        
        int giorno_attuale, aperto, minuto_attuale;
        sem_wait_operation(sem_id, SEM_MUTEX, 1);
        giorno_attuale = shm->giorno;
        aperto         = shm->giorno_aperto;
        minuto_attuale = shm->minuto_corrente;
        
        int sportelli_disponibili = shm->sportelli_per_servizio_oggi[indice];
        sem_post_operation(sem_id, SEM_MUTEX, 1);

        if (!aperto) {
            invia_risposta(msgid, (pid_t)pid_u, ESITO_NON_SERVITO_GIORNO, servizio, giorno_attuale);
            continue;
        }
        
        if (sportelli_disponibili == 0) {            
            sem_wait_operation(sem_id, SEM_MUTEX, 1);
            processa_utente_non_servito(shm, servizio);
            sem_post_operation(sem_id, SEM_MUTEX, 1);
            
            invia_risposta(msgid, (pid_t)pid_u, ESITO_NON_SERVITO_GIORNO, servizio, giorno_attuale);
            continue;
        }

        ElementoCoda e = { .pid = pid_u, .giorno = giorno_attuale, .minuto = minuto_attuale };
        sem_wait_operation(sem_id, SEM_MUTEX, 1);
        int ok = accoda(&shm->code_servizio[indice], e);
        if (ok == 0) shm->utenti_in_attesa++;
        sem_post_operation(sem_id, SEM_MUTEX, 1);

        if (ok == 0) {
            sem_post_operation(sem_id, SEM_BASE_SERVIZI + indice, 1);
        } else {
            fprintf(stderr, "[Erogatore] Coda servizio %d piena, PID %d scartato.\n", servizio, pid_u);
            
            sem_wait_operation(sem_id, SEM_MUTEX, 1);
            processa_utente_non_servito(shm, servizio);
            sem_post_operation(sem_id, SEM_MUTEX, 1);
            
            invia_risposta(msgid, (pid_t)pid_u, ESITO_NON_SERVITO_GIORNO, servizio, giorno_attuale);
        }
    }

    printf("[Erogatore] Terminazione in corso...\n");
    scollega_memoria_condivisa(shm);
    printf("[Erogatore] Terminato.\n");
    return 0;
}
