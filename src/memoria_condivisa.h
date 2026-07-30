/**
 * @file memoria_condivisa.h
 * @brief Definizione e gestione della memoria condivisa del sistema.
 */

#ifndef MEMORIA_CONDIVISA_H
#define MEMORIA_CONDIVISA_H

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stddef.h>

// Dipendenze interne
#include "semafori.h"
#include "coda.h"
#include "config.h"

// --- COSTANTI E DEFAULT ---

#ifndef FTOK_PATH
#define FTOK_PATH "/tmp/poste_shm.key"
#endif

#ifndef MAX_OPERATORI
#define MAX_OPERATORI 256
#endif

#ifndef MAX_SPORTELLI
#define MAX_SPORTELLI 128
#endif

#ifndef MAX_UTENTI_DINAMICI
#define MAX_UTENTI_DINAMICI 100
#endif

#define MAX_UTENTI_DISTINTI 2048

// --- TIPI DI DATI ---

/**
 * @enum StatoSportello
 * @brief Stati possibili di uno sportello.
 */
typedef enum {
    SPORTELLO_CHIUSO = 0,
    SPORTELLO_APERTO = 1
} StatoSportello;

/**
 * @struct Sportello
 * @brief Rappresenta un singolo sportello dell'ufficio postale.
 */
typedef struct {
    int tipo_servizio;        /**< 1..NUM_SERVIZI, -1 se non assegnato */
    StatoSportello stato;     
    pid_t pid_operatore;      /**< PID operatore, 0 se libero */
} Sportello;

/**
 * @struct SharedMemory
 * @brief Struttura principale della memoria condivisa.
 * * NOTA SULL'ALLINEAMENTO: I campi sono ordinati per mantenere un buon allineamento
 * in memoria (padding ridotto) su architetture a 64 bit.
 */
typedef struct {
    Config config;           /**< Configurazione (copia statica) */

    // Blocchi da 4 byte (int) - Totale 24 byte (allineato a 8)
    int giorno;              
    int minuto_corrente;     
    int attivo;              
    int motivo_termine;      
    int giorno_aperto;       
    int msgqueue_id;         

    // Blocchi complessi o a 8 byte
    Coda code_servizio[NUM_SERVIZI]; 

    // Statistiche temporali (8 byte)
    long long somma_attese_oggi;            
    long long somma_durata_servizi_oggi;    
    long long somma_attese_totali;                 
    long long somma_durata_servizi_totali;         

    // Array e strutture dati
    Sportello sportelli[MAX_SPORTELLI]; 
    int num_sportelli_allocati;         

    // Contatori sincronizzazione
    int operatori_inizializzati; 
    int utenti_inizializzati;    
    int erogatore_inizializzato; 

    // Statistiche Giornaliere
    int utenti_serviti_oggi;                
    int servizi_completati_oggi[NUM_SERVIZI];     
    int servizi_non_completati_oggi[NUM_SERVIZI]; 
    int utenti_in_attesa;                   
    int operatori_attivi_oggi;              
    int pause_oggi;                         

    // Tracking Operatori
    int operatori_lavorato_oggi[MAX_OPERATORI]; 
    int operatori_con_sportello[MAX_OPERATORI]; 

    // Statistiche Totali
    int totale_utenti_serviti;                     
    int totale_servizi_completati[NUM_SERVIZI];   
    int totale_servizi_non_completati[NUM_SERVIZI]; 
    int pause_totali;                              

    int operatori_attivi_sim_set[MAX_OPERATORI]; 
    int operatori_attivi_sim;                     

    int operatori_per_servizio[NUM_SERVIZI];      
    int sportelli_per_servizio_oggi[NUM_SERVIZI]; 

    // Liste PID (Statistiche uniche)
    int utenti_distinti_serviti_oggi;           
    int utenti_serviti_list_count;              
    pid_t utenti_serviti_list[MAX_UTENTI_DISTINTI]; 

    int totale_utenti_distinti;                 
    int utenti_globali_list_count;              
    pid_t utenti_globali_list[MAX_UTENTI_DISTINTI]; 
    
    // Gestione Processi
    int num_processi_utente_totali;            
    int num_max_operatori;                     
    int operatori_fine_giorno;                 

    // Utenti Dinamici
    int num_utenti_dinamici;                   
    pid_t utenti_dinamici[MAX_UTENTI_DINAMICI]; 

} SharedMemory;

// --- PROTOTIPI ---

int inizializza_memoria_condivisa(void);
int ottieni_memoria_condivisa(void);
int prova_ottieni_memoria_condivisa(void);
SharedMemory* collega_memoria_condivisa(int shmid);
void scollega_memoria_condivisa(SharedMemory* shm);
void rimuovi_memoria_condivisa(int shmid);

#endif /* MEMORIA_CONDIVISA_H */