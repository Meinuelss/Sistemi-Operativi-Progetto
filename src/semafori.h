/**
 * @file semafori.h
 * @brief Gestione dei semafori System V per la sincronizzazione tra processi.
 */

#ifndef SEMAFORI_H
#define SEMAFORI_H

#include <sys/types.h>

// Fallback di sicurezza se config.h non è stato incluso prima
#ifndef NUM_SERVIZI
#define NUM_SERVIZI 6
#endif

/**
 * @enum Indici dei semafori nel set condiviso
 */
enum {
    /**
     * @brief Mutex per la mutua esclusione sulla memoria condivisa.
     * Valore iniziale: 1 (sbloccato).
     */
    SEM_MUTEX = 0,
    
    /**
     * @brief Barriera di inizializzazione per coordinare l'avvio dei processi.
     * Valore iniziale: 0.
     */
    SEM_INIT_BARRIER = 1,

    /**
     * @brief Semaforo per segnalare l'inizio di una giornata.
     * Valore iniziale: 0.
     */
    SEM_DAY_START = 2,
    
    /**
     * @brief Semaforo per segnalare la fine di una giornata.
     * Valore iniziale: 0.
     */
    SEM_DAY_END   = 3,
    
    /**
     * @brief Barriera per coordinare la fine delle operazioni giornaliere degli operatori.
     * Valore iniziale: 0.
     */
    SEM_OPERATORI_FINE_GIORNO = 4,

    /**
     * @brief Base degli indici per i semafori per-servizio.
     * Range: [15, 20]
     */
    SEM_BASE_SERVIZI = 15,
    
    /**
     * @brief Base degli indici per i semafori di sostituzione operatori.
     * Range: [30, 35]
     */
    SEM_SOSTITUZIONE_BASE = 30
};

/**
 * @brief Crea e inizializza il set di semafori condiviso.
 * @return ID del set di semafori creato/ottenuto
 */
int  inizializza_semafori(void);

/**
 * @brief Ottiene l'ID di un set di semafori esistente.
 * @return ID del set di semafori esistente
 */
int  ottieni_semafori(void);

/**
 * @brief Rimuove definitivamente il set di semafori dal sistema.
 * @param semid ID del set di semafori da rimuovere
 */
void rimuovi_semafori(int semid);

/**
 * @brief Operazione wait (P) su un semaforo (decrementa).
 * @param semid ID del set di semafori
 * @param semnum Indice del semaforo nel set
 * @param count Quantità da decrementare (solitamente 1)
 * @return 0 successo, -1 errore
 */
int  sem_wait_operation(int semid, int semnum, int count);

/**
 * @brief Operazione post (V) su un semaforo (incrementa).
 * @param semid ID del set di semafori
 * @param semnum Indice del semaforo nel set
 * @param count Quantità da incrementare (solitamente 1)
 * @return 0 successo, -1 errore
 */
int  sem_post_operation(int semid, int semnum, int count);

#endif /* SEMAFORI_H */