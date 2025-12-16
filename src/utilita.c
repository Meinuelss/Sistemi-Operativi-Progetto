#include "utilita.h"
#include "memoria_condivisa.h"
#include "config.h"
#include "semafori.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


void crea_sportelli(SharedMemory* shm, int num_sportelli) {
    if (num_sportelli < 0) num_sportelli = 0;
    if (num_sportelli > MAX_SPORTELLI) num_sportelli = MAX_SPORTELLI;

    shm->num_sportelli_allocati = num_sportelli;
    
    for (int i = 0; i < MAX_SPORTELLI; i++) {
        shm->sportelli[i].tipo_servizio = -1;
        shm->sportelli[i].stato = SPORTELLO_CHIUSO;
        shm->sportelli[i].pid_operatore = 0;
    }
}

int trova_sportello_libero_per_servizio(SharedMemory* shm, int tipo_servizio) {
    if (tipo_servizio < 1 || tipo_servizio > NUM_SERVIZI) return -1;
    for (int i = 0; i < shm->num_sportelli_allocati; i++) {
        if (shm->sportelli[i].tipo_servizio == tipo_servizio &&
            shm->sportelli[i].stato == SPORTELLO_APERTO &&
            shm->sportelli[i].pid_operatore == 0) {
            return i;
        }
    }
    return -1;
}

void assegna_servizi_giornalieri(SharedMemory* shm) {
    srand((unsigned)(time(NULL) ^ (getpid()<<16)));

    for (int s = 0; s < NUM_SERVIZI; s++) {
        shm->sportelli_per_servizio_oggi[s] = 0;
    }
    for (int i = 0; i < shm->num_sportelli_allocati; i++) {
        shm->sportelli[i].stato = SPORTELLO_CHIUSO;
        shm->sportelli[i].tipo_servizio = -1;
        shm->sportelli[i].pid_operatore = 0;
    }

    int servizi_disponibili[NUM_SERVIZI];
    int n_servizi = 0;
    for (int s = 0; s < NUM_SERVIZI; s++) {
        if (shm->operatori_per_servizio[s] > 0) {
            servizi_disponibili[n_servizi++] = s;
        }
    }

    if (n_servizi == 0) {
        printf("[Direttore] Nessun operatore disponibile - nessuno sportello aperto\n");
        return;
    }

    for (int i = 0; i < shm->num_sportelli_allocati; i++) {
        int servizio_idx = rand() % n_servizi;
        int servizio = servizi_disponibili[servizio_idx];
        
        shm->sportelli[i].tipo_servizio = servizio + 1;
        shm->sportelli[i].stato = SPORTELLO_APERTO;
        shm->sportelli[i].pid_operatore = 0;
        shm->sportelli_per_servizio_oggi[servizio]++;
    }

    printf("[Direttore] Riepilogo sportelli giorno %d:\n", shm->giorno);
    for (int s = 0; s < NUM_SERVIZI; s++) {
        int operatori = shm->operatori_per_servizio[s];
        int sportelli = shm->sportelli_per_servizio_oggi[s];
        if (operatori > 0) {
            printf("Servizio %d: %d operatori, %d sportelli", s+1, operatori, sportelli);
            if (sportelli == 0) {
                printf(" - SERVIZIO NON EROGATO\n");
            } else if (sportelli < operatori) {
                printf(" - %d operatori senza sportello\n", operatori - sportelli);
            } else if (sportelli > operatori) {
                printf(" - %d sportelli in eccesso\n", sportelli - operatori);
            } else {
                printf(" - bilanciato\n");
            }
        }
    }
}

void processa_utente_non_servito(SharedMemory* shm, int servizio) {
    if (servizio >= 1 && servizio <= NUM_SERVIZI) {
        shm->servizi_non_completati_oggi[servizio - 1]++;
    }
}

void raccogli_statistiche_giornaliere(SharedMemory* shm) {
    shm->totale_utenti_serviti += shm->utenti_distinti_serviti_oggi;
    shm->somma_attese_totali += shm->somma_attese_oggi;
    shm->somma_durata_servizi_totali += shm->somma_durata_servizi_oggi;
    
    for (int s = 0; s < NUM_SERVIZI; s++) {
        shm->totale_servizi_completati[s]     += shm->servizi_completati_oggi[s];
        shm->totale_servizi_non_completati[s] += shm->servizi_non_completati_oggi[s];
    }
}

void resetta_statistiche_giornaliere(SharedMemory* shm) {
    shm->utenti_serviti_oggi = 0;
    shm->utenti_distinti_serviti_oggi = 0;
    shm->utenti_serviti_list_count = 0;
    for (int i = 0; i < NUM_SERVIZI; i++) {
        shm->servizi_completati_oggi[i]     = 0;
        shm->servizi_non_completati_oggi[i] = 0;
    }
    shm->utenti_in_attesa          = 0;
    shm->somma_attese_oggi         = 0;
    shm->somma_durata_servizi_oggi = 0;
    shm->operatori_attivi_oggi     = 0;
    shm->pause_oggi                = 0;

    for (int i = 0; i < MAX_OPERATORI; i++) {
        shm->operatori_lavorato_oggi[i] = 0;
        shm->operatori_con_sportello[i] = 0;
    }
}

void dormi_per_minuti(int minuti, int ns_per_minuto) {
    if (minuti <= 0 || ns_per_minuto <= 0) return;

    long long tot_ns = (long long)minuti * (long long)ns_per_minuto;
    struct timespec req, rem;

    req.tv_sec  = (time_t)(tot_ns / 1000000000LL);
    req.tv_nsec = (long) (tot_ns % 1000000000LL);

    while (nanosleep(&req, &rem) == -1 && errno == EINTR) {
        req = rem;
    }
}
