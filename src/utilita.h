/**
 * @file utilita.h
 * @brief Funzioni di utilità per la gestione della simulazione.
 * 
 * Questo modulo fornisce funzioni di supporto per la gestione degli sportelli,
 * l'assegnazione dei servizi, la raccolta delle statistiche e la simulazione
 * del passaggio del tempo.
 */

#ifndef UTILITA_H
#define UTILITA_H

#include "memoria_condivisa.h"

#define TEST_ERROR(val, msg) \
    do { \
        if ((val) == -1) { \
            fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__); \
            perror(msg); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

/**
 * @brief Crea e inizializza gli sportelli nell'ufficio postale.
 * 
 * Alloca il numero specificato di sportelli nella memoria condivisa,
 * inizializzandoli tutti come chiusi e non assegnati. Il numero di sportelli
 * è limitato da MAX_SPORTELLI.
 * 
 * @param shm Puntatore alla memoria condivisa
 * @param num_sportelli Numero di sportelli da creare (limitato a MAX_SPORTELLI)
 * 
 * @pre shm != NULL
 * @post Tutti gli sportelli sono inizializzati con stato SPORTELLO_CHIUSO
 * @post shm->num_sportelli_allocati contiene il numero effettivo di sportelli creati
 */
void crea_sportelli(SharedMemory* shm, int num_sportelli);

/**
 * @brief Trova uno sportello disponibile per un tipo di servizio specifico.
 * 
 * Cerca tra gli sportelli allocati uno che sia:
 * - Assegnato al tipo di servizio richiesto
 * - In stato SPORTELLO_APERTO
 * - Non occupato da un operatore (pid_operatore == 0)
 * 
 * @param shm Puntatore alla memoria condivisa
 * @param tipo_servizio Tipo di servizio cercato (1..NUM_SERVIZI)
 * @return Indice dello sportello libero (0-based), o -1 se nessuno sportello disponibile
 * 
 * @pre shm != NULL
 * @pre tipo_servizio >= 1 && tipo_servizio <= NUM_SERVIZI
 * @note Questa funzione NON è thread-safe, deve essere chiamata all'interno di una sezione critica
 */
int  trova_sportello_libero_per_servizio(SharedMemory* shm, int tipo_servizio);

/**
 * @brief Assegna casualmente i servizi agli sportelli per la giornata corrente.
 * 
 * All'inizio di ogni giorno, questa funzione:
 * 1. Azzera tutti i contatori e chiude tutti gli sportelli
 * 2. Identifica i servizi con operatori disponibili
 * 3. Assegna casualmente ogni sportello a uno dei servizi disponibili
 * 4. Apre gli sportelli assegnati
 * 5. Stampa un riepilogo dell'assegnazione
 * 
 * La distribuzione casuale simula la variabilità giornaliera dell'organizzazione
 * dell'ufficio postale. Alcuni servizi potrebbero non avere sportelli se non ci
 * sono operatori disponibili per quel servizio.
 * 
 * @param shm Puntatore alla memoria condivisa
 * 
 * @pre shm != NULL
 * @post Ogni sportello ha un tipo_servizio assegnato e stato = SPORTELLO_APERTO
 * @post sportelli_per_servizio_oggi[i] contiene il numero di sportelli per il servizio i+1
 * @note Deve essere chiamata all'inizio di ogni giornata, prima che gli operatori si attivino
 */
void assegna_servizi_giornalieri(SharedMemory* shm);

/**
 * @brief Raccoglie e accumula le statistiche giornaliere nei totali della simulazione.
 * 
 * Questa funzione deve essere chiamata alla fine di ogni giornata per:
 * - Sommare utenti_distinti_serviti_oggi al totale_utenti_serviti
 * - Sommare i tempi di attesa giornalieri ai totali
 * - Sommare i tempi di servizio giornalieri ai totali
 * - Accumulare servizi completati e non completati per ogni tipo
 * 
 * @param shm Puntatore alla memoria condivisa
 * 
 * @pre shm != NULL
 * @post I contatori totali sono aggiornati con i valori giornalieri
 * @note Non resetta i contatori giornalieri (usare resetta_statistiche_giornaliere per quello)
 */
void raccogli_statistiche_giornaliere(SharedMemory* shm);

/**
 * @brief Resetta tutti i contatori statistici giornalieri a zero.
 * 
 * Prepara la memoria condivisa per una nuova giornata, azzerando:
 * - Utenti serviti oggi
 * - Utenti distinti serviti oggi
 * - Lista utenti serviti (per tracciamento unicità)
 * - Servizi completati e non completati per ogni tipo
 * - Utenti in attesa
 * - Somme dei tempi di attesa e servizio giornalieri
 * - Contatori operatori attivi oggi
 * - Pause effettuate oggi
 * 
 * @param shm Puntatore alla memoria condivisa
 * 
 * @pre shm != NULL
 * @post Tutti i contatori giornalieri sono azzerati
 * @note Deve essere chiamata DOPO raccogli_statistiche_giornaliere
 */
void resetta_statistiche_giornaliere(SharedMemory* shm);

/**
 * @brief Aggiorna le statistiche per un utente che non è stato servito.
 * 
 * Incrementa i contatori appropriati quando un utente non può essere servito
 * (coda piena, servizio non disponibile, giorno chiuso durante il servizio).
 * Aggiorna servizi_non_completati_oggi per il tipo di servizio specificato.
 * 
 * @param shm Puntatore alla memoria condivisa
 * @param servizio Tipo di servizio richiesto (1..NUM_SERVIZI)
 * 
 * @pre shm != NULL
 * @pre servizio >= 1 && servizio <= NUM_SERVIZI
 * @post servizi_non_completati_oggi[servizio-1] è incrementato
 */
void processa_utente_non_servito(SharedMemory* shm, int servizio);

/**
 * @brief Simula il passaggio del tempo nella simulazione.
 * 
 * Sospende l'esecuzione del processo chiamante per simulare il passaggio
 * di un determinato numero di minuti simulati. Usa nanosleep() per precisione
 * e gestisce automaticamente le interruzioni (EINTR).
 * 
 * Il tempo reale di attesa è calcolato come:
 *   tempo_reale_ns = minuti * ns_per_minuto
 * 
 * @param minuti Numero di minuti simulati da attendere
 * @param ns_per_minuto Nanosecondi reali corrispondenti a 1 minuto simulato
 * 
 * @note Se minuti <= 0 o ns_per_minuto <= 0, la funzione ritorna immediatamente
 * @note Gestisce automaticamente le interruzioni da segnali (riprende il sleep rimanente)
 * @note Esempio: dormi_per_minuti(10, 30000000) attende 300ms in tempo reale
 */
void dormi_per_minuti(int minuti, int ns_per_minuto);

#endif /* UTILITA_H */
