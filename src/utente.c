#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "memoria_condivisa.h"
#include "semafori.h"
#include "messaggi.h"
#include "utilita.h"
#include "config.h"

static volatile sig_atomic_t termination_requested = 0;

static void signal_handler(int sig) {
    (void)sig;
    termination_requested = 1;
}

static double frand01(void) { 
    return (double)rand() / (double)RAND_MAX; 
}

int main(int argc, char** argv) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    TEST_ERROR(sigaction(SIGTERM, &sa, NULL), "sigaction SIGTERM");
    TEST_ERROR(sigaction(SIGINT, &sa, NULL), "sigaction SIGINT");

    int id_utente = (argc > 1) ? atoi(argv[1]) : -1;

    int shmid = ottieni_memoria_condivisa();
    if (shmid < 0) TEST_ERROR(-1, "shmget utente");

    SharedMemory* shm = collega_memoria_condivisa(shmid);
    if (!shm) TEST_ERROR(-1, "shmat utente");

    int sem_id = ottieni_semafori();
    if (sem_id < 0) TEST_ERROR(-1, "semget utente");
    
    int msgid = shm->msgqueue_id;

    sem_wait_operation(sem_id, SEM_MUTEX, 1);
    shm->utenti_inizializzati++;
    sem_post_operation(sem_id, SEM_MUTEX, 1);

    srand((unsigned)(time(NULL) ^ (getpid() << 16))); 
    
    sem_post_operation(sem_id, SEM_INIT_BARRIER, 1);

    double pmin = (shm->config.p_min >= 0.0) ? shm->config.p_min : 0.3;
    double pmax = (shm->config.p_max >= 0.0) ? shm->config.p_max : 0.7;
    if (pmax < pmin) { double t = pmin; pmin = pmax; pmax = t; }
    
    double p_serv = pmin + frand01() * (pmax - pmin);

    while (shm->attivo && shm->giorno <= shm->config.durata_sim && !termination_requested) {
        
        if (sem_wait_operation(sem_id, SEM_DAY_START, 1) < 0) break;
        if (!shm->attivo || termination_requested) break;

        int servizi_attivi_oggi[NUM_SERVIZI]; 
        int num_attivi = 0;

        sem_wait_operation(sem_id, SEM_MUTEX, 1);
        int giorno_corrente = shm->giorno;
        for (int s = 0; s < NUM_SERVIZI; ++s) {
            servizi_attivi_oggi[s] = (shm->sportelli_per_servizio_oggi[s] > 0) ? 1 : 0;
            if (servizi_attivi_oggi[s]) num_attivi++;
        }
        sem_post_operation(sem_id, SEM_MUTEX, 1);
        
        if (num_attivi == 0) {
            if (sem_wait_operation(sem_id, SEM_DAY_END, 1) < 0) break;
            continue;
        }

        double random_val = frand01();
        int vado_in_ufficio = (random_val <= p_serv);
        
        printf("[Utente %d] Giorno %d: %s\n", id_utente, giorno_corrente, 
               vado_in_ufficio ? "Va all'ufficio" : "Resta a casa");

        if (vado_in_ufficio) {
            
            int active_services_indices[NUM_SERVIZI];
            int count_active = 0;
            for (int s = 0; s < NUM_SERVIZI; s++) {
                if (servizi_attivi_oggi[s]) {
                    active_services_indices[count_active++] = s + 1;
                }
            }

            int tipo_servizio_scelto = (rand() % NUM_SERVIZI) + 1;
            int tempo_arrivo = rand() % shm->config.durata_giorno;
            
            int max_req = (shm->config.num_richieste < 1 ? 1 : shm->config.num_richieste);
            int req_count = 1 + (rand() % max_req);
            
            printf("[Utente %d] Giorno %d: Arriverà al min %d per S%d (%d richieste)\n",
                   id_utente, giorno_corrente, tempo_arrivo, tipo_servizio_scelto, req_count);
            
            dormi_per_minuti(tempo_arrivo, shm->config.ns_per_minuto);
            
            
            if (shm->attivo && shm->giorno_aperto && !termination_requested) {
                sem_wait_operation(sem_id, SEM_MUTEX, 1);
                int servizio_aperto = (shm->sportelli_per_servizio_oggi[tipo_servizio_scelto - 1] > 0);
                sem_post_operation(sem_id, SEM_MUTEX, 1);

                if (servizio_aperto) {
                    
                    int *servizi_da_richiedere = malloc(req_count * sizeof(int));
                    if (!servizi_da_richiedere) TEST_ERROR(-1, "malloc servizi");

                    servizi_da_richiedere[0] = tipo_servizio_scelto;
                    
                    for (int i = 1; i < req_count; i++) {
                        if (count_active > 0)
                            servizi_da_richiedere[i] = active_services_indices[rand() % count_active];
                        else 
                            servizi_da_richiedere[i] = (rand() % NUM_SERVIZI) + 1;
                    }
                    
                    for (int r = 0; r < req_count; ++r) {
                        if (!shm->attivo || !shm->giorno_aperto || termination_requested) break;

                        MessaggioRichiesta req;
                        req.mtype = TIPO_MSG_RICHIESTA;
                        req.pid_utente = getpid();
                        req.tipo_servizio = servizi_da_richiedere[r];

                        if (msgsnd(msgid, &req, sizeof(req) - sizeof(long), 0) == -1) {
                            if (errno != EINTR && errno != EIDRM) 
                                TEST_ERROR(-1, "msgsnd richiesta");
                        } else {
                            MessaggioRisposta resp;
                            int res = ricevi_risposta_per_pid(msgid, &resp, 0);
                            
                            if (res == -1) {
                                if (errno != EINTR && errno != EIDRM)
                                    perror("msgrcv risposta");
                            } else if (resp.esito == ESITO_NON_SERVITO_GIORNO) {
                                printf("[Utente %d] Giorno %d: S%d chiuso/pieno. Fine richieste.\n",
                                       id_utente, giorno_corrente, servizi_da_richiedere[r]);
                                break; 
                            } else if (resp.esito == ESITO_SERVITO) {
                                printf("[Utente %d] Giorno %d: Servito S%d (req %d/%d)\n",
                                       id_utente, giorno_corrente, servizi_da_richiedere[r], r + 1, req_count);
                            }
                        }
                    }
                    
                    // IMPORTANTE: Liberare la memoria
                    free(servizi_da_richiedere);

                } else {
                    printf("[Utente %d] Giorno %d: Trovato S%d chiuso all'arrivo.\n",
                           id_utente, giorno_corrente, tipo_servizio_scelto);
                }
            }
        }        
    
        if (sem_wait_operation(sem_id, SEM_DAY_END, 1) < 0) break;
    }

    scollega_memoria_condivisa(shm);
    return 0;
}