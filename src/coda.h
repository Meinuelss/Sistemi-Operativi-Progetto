/**
 * @file coda.h
 * @brief Implementazione di una coda circolare (FIFO) per la gestione delle richieste utente.
 * 
 * Questo modulo fornisce una struttura dati coda circolare thread-safe (se usata con semafori),
 * utilizzata per mantenere le richieste in attesa per ciascun tipo di servizio.
 * La coda ha capacità fissa e supporta operazioni di inserimento ed estrazione.
 */

#ifndef CODA_H
#define CODA_H

/**
 * @def LUNGHEZZA_MAX_CODA
 * @brief Capacità massima della coda circolare.
 * 
 * Definisce il numero massimo di elementi che possono essere contenuti nella coda.
 * Quando la coda è piena, nuove richieste vengono rifiutate.
 */
#define LUNGHEZZA_MAX_CODA 100

/**
 * @struct ElementoCoda
 * @brief Rappresenta un singolo elemento nella coda di richieste.
 * 
 * Ogni elemento contiene le informazioni necessarie per identificare
 * l'utente richiedente e calcolare i tempi di attesa.
 */
typedef struct {
    int pid;       /**< PID del processo utente che ha effettuato la richiesta */
    int giorno;    /**< Giorno della simulazione in cui è stata fatta la richiesta (1-based) */
    int minuto;    /**< Minuto di arrivo nella giornata simulata (0..durata_giorno-1) */
} ElementoCoda;

/**
 * @struct Coda
 * @brief Struttura della coda circolare FIFO.
 * 
 * Implementa una coda circolare con capacità fissa. Gli indici testa e coda
 * vengono gestiti con aritmetica modulo per implementare il comportamento circolare.
 */
typedef struct {
    ElementoCoda elementi[LUNGHEZZA_MAX_CODA]; /**< Array circolare degli elementi */
    int testa;      /**< Indice del primo elemento da estrarre (front) */
    int coda;       /**< Indice dove inserire il prossimo elemento (rear) */
    int conteggio;  /**< Numero corrente di elementi nella coda */
} Coda;

/**
 * @brief Inizializza una coda vuota.
 * 
 * Imposta tutti i contatori della coda a zero, preparandola per l'uso.
 * Deve essere chiamata prima di qualsiasi altra operazione sulla coda.
 * 
 * @param coda Puntatore alla struttura Coda da inizializzare
 * 
 * @pre coda != NULL
 * @post La coda è vuota (conteggio == 0)
 */
void inizializza_coda(Coda* coda);

/**
 * @brief Inserisce un elemento in coda (enqueue).
 * 
 * Aggiunge un nuovo elemento alla fine della coda. Se la coda è piena,
 * l'operazione fallisce e restituisce un errore.
 * 
 * @param coda Puntatore alla coda in cui inserire
 * @param valore Elemento da inserire nella coda
 * @return 0 in caso di successo, -1 se la coda è piena
 * 
 * @pre coda != NULL
 * @post Se successo, conteggio incrementato di 1
 */
int accoda(Coda* coda, ElementoCoda valore);

/**
 * @brief Estrae un elemento dalla coda (dequeue).
 * 
 * Rimuove e restituisce l'elemento in testa alla coda (FIFO: First In, First Out).
 * Se la coda è vuota, l'operazione fallisce.
 * 
 * @param coda Puntatore alla coda da cui estrarre
 * @param valore Puntatore dove memorizzare l'elemento estratto
 * @return 0 in caso di successo, -1 se la coda è vuota
 * 
 * @pre coda != NULL && valore != NULL
 * @post Se successo, conteggio decrementato di 1 e *valore contiene l'elemento estratto
 */
int estrai_da_coda(Coda* coda, ElementoCoda* valore);

/**
 * @brief Verifica se la coda è vuota.
 * 
 * @param coda Puntatore alla coda da verificare
 * @return 1 se la coda è vuota, 0 altrimenti
 * 
 * @pre coda != NULL
 */
int coda_vuota(Coda* coda);

/**
 * @brief Verifica se la coda è piena.
 * 
 * @param coda Puntatore alla coda da verificare
 * @return 1 se la coda è piena (conteggio == LUNGHEZZA_MAX_CODA), 0 altrimenti
 * 
 * @pre coda != NULL
 */
int coda_piena(Coda* coda);

#endif /* CODA_H */
