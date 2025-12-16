#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <errno.h>

#include "memoria_condivisa.h"
#include "semafori.h"
#include "utilita.h"
#include "config.h"
#include "messaggi.h"
#include "coda.h"
#include "statistiche.h"



static void svuota_code_servizi(SharedMemory* shm, int sem_id) {
    for (int s = 0; s < NUM_SERVIZI; s++) {
        while (1) {
            ElementoCoda e;
            sem_wait_operation(sem_id, SEM_MUTEX, 1);
            int ok = estrai_da_coda(&shm->code_servizio[s], &e);
            
            if (ok == 0) {
                if (shm->utenti_in_attesa > 0) shm->utenti_in_attesa--;
                processa_utente_non_servito(shm, s + 1);
            }
            sem_post_operation(sem_id, SEM_MUTEX, 1);

            if (ok != 0) break;
            invia_risposta(shm->msgqueue_id, (pid_t)e.pid, ESITO_NON_SERVITO_GIORNO, s + 1, shm->giorno);
        }
    }
}

static void svuota_coda_messaggi_non_serviti(SharedMemory* shm) {
    while (1) {
        MessaggioRichiesta req;
        ssize_t n = msgrcv(shm->msgqueue_id, &req, sizeof(req) - sizeof(long),
                           TIPO_MSG_RICHIESTA, IPC_NOWAIT);
        if (n == -1) {
            if (errno == ENOMSG) break;
            TEST_ERROR(-1, "[Direttore] msgrcv cleanup");
        }
        invia_risposta(shm->msgqueue_id, req.pid_utente, ESITO_NON_SERVITO_GIORNO,
                      req.tipo_servizio, shm->giorno);
    }
}

void termina_processi_figli(pid_t erogatore, pid_t* operatori, int n_op, pid_t* utenti, int n_ut, pid_t* dinamici, int n_din) {
    printf("[Direttore] Invio segnali di terminazione...\n");
    
    if (erogatore > 0) kill(erogatore, SIGTERM);
    
    for (int i = 0; i < n_op; i++)
        if (operatori[i] > 0) kill(operatori[i], SIGTERM);
        
    for (int i = 0; i < n_ut; i++)
        if (utenti[i] > 0) kill(utenti[i], SIGTERM);
        
    for (int i = 0; i < n_din; i++)
        if (dinamici[i] > 0) kill(dinamici[i], SIGTERM);

    printf("[Direttore] Attesa terminazione (wait)...\n");

    while (wait(NULL) > 0); 
    printf("[Direttore] Tutti i processi terminati.\n");
}


int main(int argc, char** argv) {

    srand((unsigned)(time(NULL) ^ (getpid()<<16))); 

    int shm_id = inizializza_memoria_condivisa();
    SharedMemory* shm = collega_memoria_condivisa(shm_id);

    int sem_id = inizializza_semafori();
    int msg_id = crea_coda_messaggi();
    shm->msgqueue_id = msg_id;

    int num_operatori, num_utenti, num_sportelli;
    int durata_sim, ns_per_minuto, max_pause;
    float p_min, p_max;
    int soglia_attesa, durata_giorno, n_requests;

    const char* cfg_path = risolvi_percorso_config(argc, argv);
    carica_configurazione(cfg_path,
                &num_operatori, &num_utenti, &num_sportelli,
                &durata_sim, &ns_per_minuto, &max_pause, &p_min, &p_max,
                &soglia_attesa, &durata_giorno, &n_requests);

    shm->config = (Config){NUM_SERVIZI, durata_giorno, soglia_attesa, ns_per_minuto,
                           durata_sim, p_min, p_max, max_pause, n_requests};

    inizializza_file_csv();

    for (int i = 1; i <= num_operatori; ++i) {
        int serv = ((i-1) % NUM_SERVIZI) + 1;
        shm->operatori_per_servizio[serv - 1]++;
    }
    
    printf("[Direttore] Configurazione: %d Op, %d Utenti, %d Sportelli\n", 
            num_operatori, num_utenti, num_sportelli);
    
    memset(shm->operatori_attivi_sim_set, 0, sizeof(shm->operatori_attivi_sim_set));
    shm->operatori_attivi_sim = 0;
    shm->num_max_operatori = num_operatori;
    crea_sportelli(shm, num_sportelli);

    shm->operatori_inizializzati = 0;
    shm->utenti_inizializzati = 0;
    shm->erogatore_inizializzato = 0;
    shm->num_processi_utente_totali = num_utenti;

    pid_t *pids_operatori = calloc(num_operatori, sizeof(pid_t));
    pid_t *pids_utenti = calloc(num_utenti, sizeof(pid_t));
    pid_t pid_erogatore = -1;

    if (!pids_operatori || !pids_utenti) TEST_ERROR(-1, "calloc pids");

    for (int i = 0; i < num_operatori; i++) {
        pid_t pid = fork();
        TEST_ERROR(pid, "fork operatore");
        if (pid == 0) {
            char id[16]; snprintf(id, sizeof(id), "%d", i + 1);
            execl(OPERATORE_EXEC, OPERATORE_EXEC, id, (char*)NULL);
            TEST_ERROR(-1, "execl operatore fallita");
        } 
        pids_operatori[i] = pid;
    }

    pid_t pid = fork();
    TEST_ERROR(pid, "fork erogatore");
    if (pid == 0) {
        execl(EROGATORE_TICKET_EXEC, EROGATORE_TICKET_EXEC, (char*)NULL);
        TEST_ERROR(-1, "execl erogatore fallita");
    }
    pid_erogatore = pid;

    for (int i = 0; i < num_utenti; i++) {
        pid = fork();
        TEST_ERROR(pid, "fork utente");
        if (pid == 0) {
            char id[16]; snprintf(id, sizeof(id), "%d", i + 1);
            execl(UTENTE_EXEC, UTENTE_EXEC, id, (char*)NULL);
            TEST_ERROR(-1, "execl utente fallita");
        }
        pids_utenti[i] = pid;
    }

    sem_wait_operation(sem_id, SEM_INIT_BARRIER, num_operatori + num_utenti + 1);

    printf("[Direttore] Simulazione avviata.\n");
    shm->attivo = 1;

    int motivo_termine = 0;
    int num_in_attesa;

    for (int g = 0; g < durata_sim && shm->attivo; g++) {
        shm->giorno = g + 1;
        shm->minuto_corrente = 0;

        sem_wait_operation(sem_id, SEM_MUTEX, 1);
        int num_dinamici = shm->num_utenti_dinamici;
        sem_post_operation(sem_id, SEM_MUTEX, 1);
        
        num_in_attesa = num_utenti + num_operatori + num_dinamici;

        sem_wait_operation(sem_id, SEM_MUTEX, 1);
        shm->operatori_fine_giorno = 0;
        shm->giorno_aperto = 1;
        sem_post_operation(sem_id, SEM_MUTEX, 1);
        
        printf("\n=== Giorno %d ===\n", shm->giorno);
        assegna_servizi_giornalieri(shm);

        for (int s = 0; s < NUM_SERVIZI; s++) 
            semctl(sem_id, SEM_SOSTITUZIONE_BASE + s, SETVAL, 0);

        sem_post_operation(sem_id, SEM_DAY_START, num_in_attesa);

        for (int m = 0; m < shm->config.durata_giorno && shm->attivo; m++) {
            sem_wait_operation(sem_id, SEM_MUTEX, 1);
            shm->minuto_corrente = m;
            sem_post_operation(sem_id, SEM_MUTEX, 1);
            dormi_per_minuti(1, shm->config.ns_per_minuto);
        }

        sem_wait_operation(sem_id, SEM_MUTEX, 1);
        shm->giorno_aperto = 0;
        if (shm->utenti_in_attesa > shm->config.soglia_attesa) {
            shm->attivo = 0;
            motivo_termine = 2;
        }
        sem_post_operation(sem_id, SEM_MUTEX, 1);

        for (int s = 0; s < NUM_SERVIZI; s++)
            sem_post_operation(sem_id, SEM_BASE_SERVIZI + s, num_operatori);

        svuota_code_servizi(shm, sem_id);
        svuota_coda_messaggi_non_serviti(shm);

        for (int s = 0; s < NUM_SERVIZI; s++)
            sem_post_operation(sem_id, SEM_SOSTITUZIONE_BASE + s, num_operatori);

        sem_post_operation(sem_id, SEM_DAY_END, num_in_attesa);
        sem_wait_operation(sem_id, SEM_OPERATORI_FINE_GIORNO, num_operatori);

        raccogli_statistiche_giornaliere(shm);
        printf("[Direttore] Giorno %d completato.\n", shm->giorno);
        stampa_statistiche(shm, 0, motivo_termine);
        resetta_statistiche_giornaliere(shm);
    }

    if (motivo_termine == 0) motivo_termine = 1;

    sem_wait_operation(sem_id, SEM_MUTEX, 1);
    shm->motivo_termine = motivo_termine;
    shm->attivo = 0;
    shm->giorno_aperto = 0;
    int tot_finale = num_utenti + num_operatori + shm->num_utenti_dinamici;
    sem_post_operation(sem_id, SEM_MUTEX, 1);

    sem_post_operation(sem_id, SEM_DAY_START, tot_finale);
    sem_post_operation(sem_id, SEM_DAY_END, tot_finale);
    
    for (int s = 0; s < NUM_SERVIZI; s++)
        sem_post_operation(sem_id, SEM_BASE_SERVIZI + s, 128);

    svuota_coda_messaggi_non_serviti(shm);

    termina_processi_figli(pid_erogatore, pids_operatori, num_operatori, 
                          pids_utenti, num_utenti, 
                          shm->utenti_dinamici, shm->num_utenti_dinamici);

    stampa_statistiche(shm, 1, motivo_termine);

    rimuovi_coda_messaggi(msg_id);
    rimuovi_semafori(sem_id);
    rimuovi_memoria_condivisa(shm_id);

    free(pids_operatori);
    free(pids_utenti);
    
    return 0;
}