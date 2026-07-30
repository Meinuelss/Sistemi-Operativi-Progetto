#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <errno.h>

#include "memoria_condivisa.h"
#include "semafori.h"
#include "messaggi.h"
#include "coda.h"
#include "config.h"
#include "utilita.h"
#include "servizi.h"

static volatile sig_atomic_t termination_requested = 0;

static void signal_handler(int sig) {
    (void)sig;
    termination_requested = 1;
}

static double frand01(void) { 
    return (double)rand() / (double)RAND_MAX; 
}

static bool pid_in_array(pid_t pid, const pid_t* array, int count) {
    for (int i = 0; i < count; i++) {
        if (array[i] == pid) return true;
    }
    return false;
}

static int acquisisci_sportello(SharedMemory* shm, int sem_id, int servizio, int id_operatore) {
    int indice_libero = -1;
    
    if (termination_requested || !shm->attivo || !shm->giorno_aperto) 
        return -1;

    sem_wait_operation(sem_id, SEM_MUTEX, 1);
    
    indice_libero = trova_sportello_libero_per_servizio(shm, servizio);
    
    if (indice_libero >= 0) {
        shm->sportelli[indice_libero].pid_operatore = getpid();
        
        if (id_operatore >= 0 && id_operatore < MAX_OPERATORI) {
            if (shm->operatori_lavorato_oggi[id_operatore] == 0) {
                shm->operatori_attivi_oggi++;
                shm->operatori_lavorato_oggi[id_operatore] = 1;
            }
            shm->operatori_con_sportello[id_operatore] = 1;
            
            if (shm->operatori_attivi_sim_set[id_operatore] == 0) {
                shm->operatori_attivi_sim_set[id_operatore] = 1;
                shm->operatori_attivi_sim++;
            }
        }
    }
    
    sem_post_operation(sem_id, SEM_MUTEX, 1);
    return indice_libero;
}

static void libera_sportello(SharedMemory* shm, int sem_id, int mio_sportello, bool notifica_sostituzione, int id_operatore) {
    if (mio_sportello < 0) return;

    int servizio = -1;
    sem_wait_operation(sem_id, SEM_MUTEX, 1);
    
    if (shm->sportelli[mio_sportello].pid_operatore == getpid()) {
        servizio = shm->sportelli[mio_sportello].tipo_servizio;
        shm->sportelli[mio_sportello].pid_operatore = 0; 
        
        if (id_operatore >= 0 && id_operatore < MAX_OPERATORI) {
            shm->operatori_con_sportello[id_operatore] = 0;
        }
    }
    
    sem_post_operation(sem_id, SEM_MUTEX, 1);

    if (notifica_sostituzione && servizio > 0) {
        sem_post_operation(sem_id, SEM_SOSTITUZIONE_BASE + servizio - 1, 1);
    }
}

int main(int argc, char** argv) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; 
    
    TEST_ERROR(sigaction(SIGTERM, &sa, NULL), "sigaction SIGTERM");
    TEST_ERROR(sigaction(SIGINT, &sa, NULL), "sigaction SIGINT");

    int id_operatore = (argc > 1) ? atoi(argv[1]) : 0;
    srand((unsigned)(time(NULL) ^ (getpid() << 16)));

    int shmid = ottieni_memoria_condivisa(); 
    if (shmid < 0) TEST_ERROR(-1, "shmget operatore"); 
    
    SharedMemory* shm = collega_memoria_condivisa(shmid);
    if (!shm) TEST_ERROR(-1, "shmat operatore");

    int sem_id = ottieni_semafori();
    if (sem_id < 0) TEST_ERROR(-1, "semget operatore");
    
    int msgid = shm->msgqueue_id;

    printf("[Operatore %d] Avviato (PID %d).\n", id_operatore, getpid());

    sem_wait_operation(sem_id, SEM_MUTEX, 1);
    shm->operatori_inizializzati++;
    sem_post_operation(sem_id, SEM_MUTEX, 1);
    
    sem_post_operation(sem_id, SEM_INIT_BARRIER, 1);

    const int mio_servizio = ((id_operatore > 0) ? ((id_operatore-1) % NUM_SERVIZI) + 1 : 1);
    int pause_totali = 0;

    while (shm->attivo && !termination_requested) {
        
        if (sem_wait_operation(sem_id, SEM_DAY_START, 1) < 0 || !shm->attivo || termination_requested) break;
        
        int mio_sportello = acquisisci_sportello(shm, sem_id, mio_servizio, id_operatore);
        
        if (mio_sportello >= 0) {
            printf("[Operatore %d] Giorno %d: Inizio turno allo sportello %d (Servizio %d)\n",
                   id_operatore, shm->giorno, mio_sportello+1, mio_servizio);
        } else {
            while (shm->attivo && shm->giorno_aperto && !termination_requested) {
                sem_wait_operation(sem_id, SEM_SOSTITUZIONE_BASE + mio_servizio - 1, 1);
                
                if (shm->attivo == 0 || shm->giorno_aperto == 0) break;
                
                mio_sportello = acquisisci_sportello(shm, sem_id, mio_servizio, id_operatore);
                
                if (mio_sportello >= 0) {
                    printf("[Operatore %d] Giorno %d: Subentro sportello %d\n",
                           id_operatore, shm->giorno, mio_sportello + 1);
                    break;
                }
            }
            
            if (mio_sportello == -1) {
                printf("[Operatore %d] Giorno %d: Nessuno sportello libero, torno a casa.\n", 
                       id_operatore, shm->giorno);
                
                sem_wait_operation(sem_id, SEM_MUTEX, 1);
                shm->operatori_fine_giorno++;
                sem_post_operation(sem_id, SEM_MUTEX, 1);
                
                sem_post_operation(sem_id, SEM_OPERATORI_FINE_GIORNO, 1);
                sem_wait_operation(sem_id, SEM_DAY_END, 1);
                continue; 
            }
        }

        while (shm->attivo && shm->giorno_aperto && mio_sportello >= 0 && !termination_requested) {

            int current_idx = mio_servizio - 1;

            if (sem_wait_operation(sem_id, SEM_BASE_SERVIZI + current_idx, 1) < 0) break;
            
            if (!shm->attivo || !shm->giorno_aperto || termination_requested) break;

            ElementoCoda q;
            sem_wait_operation(sem_id, SEM_MUTEX, 1);
            int ok = estrai_da_coda(&shm->code_servizio[current_idx], &q);
            if (ok == 0) shm->utenti_in_attesa--;
            int minuto_now = shm->minuto_corrente;
            sem_post_operation(sem_id, SEM_MUTEX, 1);

            if (ok != 0) continue; 

            int attesa = (q.giorno == shm->giorno) ? (minuto_now - q.minuto) : 0;
            if (attesa < 0) attesa = 0;

            int base = minuti_base_servizio(mio_servizio);
            int durata = (int)llround(base * (0.5 + frand01()));
            if (durata < 1) durata = 1;

            sem_wait_operation(sem_id, SEM_MUTEX, 1);
            bool nuovo_utente_oggi = !pid_in_array(q.pid, shm->utenti_serviti_list, shm->utenti_serviti_list_count);
            shm->somma_attese_oggi += attesa;
            sem_post_operation(sem_id, SEM_MUTEX, 1);

            dormi_per_minuti(durata, shm->config.ns_per_minuto);

            sem_wait_operation(sem_id, SEM_MUTEX, 1);
            if (shm->giorno_aperto) {
                shm->servizi_completati_oggi[current_idx]++;
                shm->somma_durata_servizi_oggi += durata;
                
                if (nuovo_utente_oggi) {
                    if (shm->utenti_serviti_list_count < MAX_UTENTI_DISTINTI) {
                        shm->utenti_serviti_list[shm->utenti_serviti_list_count++] = q.pid;
                        shm->utenti_distinti_serviti_oggi++;
                        shm->utenti_serviti_oggi++;
                    }
                }
                invia_risposta(msgid, (pid_t)q.pid, ESITO_SERVITO, mio_servizio, shm->giorno);
            } else {
                shm->servizi_non_completati_oggi[current_idx]++;
                invia_risposta(msgid, (pid_t)q.pid, ESITO_NON_SERVITO_GIORNO, mio_servizio, shm->giorno);
            }
            sem_post_operation(sem_id, SEM_MUTEX, 1);

            if (mio_sportello >= 0 && pause_totali < shm->config.max_pause && 
                frand01() < 0.30 && shm->attivo && shm->giorno_aperto) {
                
                pause_totali++;
                printf("[Operatore %d] Giorno %d: PAUSA (%d/%d). Rilascio sportello.\n", 
                       id_operatore, shm->giorno, pause_totali, shm->config.max_pause);
                
                sem_wait_operation(sem_id, SEM_MUTEX, 1);
                shm->pause_oggi++;
                shm->pause_totali++;
                sem_post_operation(sem_id, SEM_MUTEX, 1);
                
                libera_sportello(shm, sem_id, mio_sportello, true, id_operatore);
                mio_sportello = -1;
                break; 
            }
        }

        libera_sportello(shm, sem_id, mio_sportello, false, id_operatore);
        
        sem_wait_operation(sem_id, SEM_MUTEX, 1);
        shm->operatori_fine_giorno++;
        sem_post_operation(sem_id, SEM_MUTEX, 1);
        
        sem_post_operation(sem_id, SEM_OPERATORI_FINE_GIORNO, 1);

        if (sem_wait_operation(sem_id, SEM_DAY_END, 1) < 0) break;
    }

    scollega_memoria_condivisa(shm);
    printf("[Operatore %d] Terminazione.\n", id_operatore);
    return 0;
}