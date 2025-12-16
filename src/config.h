/**
 * @file config.h
 * @brief Gestione della configurazione della simulazione dell'ufficio postale.
 * 
 * Questo modulo fornisce le funzioni per caricare e risolvere la configurazione
 * della simulazione da file esterno (config.conf) o variabili d'ambiente.
 * Include anche la definizione della struttura Config condivisa tra i processi.
 */

#ifndef CONFIG_H
#define CONFIG_H

/**
 * @def EROGATORE_TICKET_EXEC
 * @brief Percorso dell'eseguibile del processo erogatore ticket.
 */
#define EROGATORE_TICKET_EXEC "./bin/erogatore_ticket"

/**
 * @def OPERATORE_EXEC
 * @brief Percorso dell'eseguibile del processo operatore.
 */
#define OPERATORE_EXEC        "./bin/operatore"

/**
 * @def UTENTE_EXEC
 * @brief Percorso dell'eseguibile del processo utente.
 */
#define UTENTE_EXEC           "./bin/utente"

/**
 * @def PERCORSO_FILE_CONFIG
 * @brief Percorso predefinito del file di configurazione.
 */
#define PERCORSO_FILE_CONFIG "config.conf"

/**
 * @def NUM_SERVIZI
 * @brief Numero fisso di tipologie di servizi disponibili nell'ufficio postale.
 * 
 * Il sistema supporta esattamente 6 tipi di servizi differenti:
 * 1. Invio e ritiro pacchi
 * 2. Invio e ritiro lettere raccomandate
 * 3. Prelievi e pagamenti bancoposta
 * 4. Pagamento bollettini postali
 * 5. Acquisto prodotti finanziari
 * 6. Acquisto orologi e braccialetti
 */
#ifndef NUM_SERVIZI
#define NUM_SERVIZI 6
#endif

/**
 * @def N_REQUEST
 * @brief Numero massimo di richieste che un singolo utente può effettuare in una giornata.
 */
#ifndef N_REQUEST
#define N_REQUEST 10
#endif

/**
 * @struct Config
 * @brief Struttura contenente tutti i parametri di configurazione della simulazione.
 * 
 * Questa struttura viene memorizzata nella memoria condivisa e contiene tutti
 * i parametri necessari per l'esecuzione della simulazione, caricati dal file config.conf.
 */
typedef struct {
    int   num_servizi;         /**< Numero di servizi disponibili (= NUM_SERVIZI = 6) */
    int   durata_giorno;       /**< Durata di una giornata in minuti simulati (DAY_DURATION) */
    int   soglia_attesa;       /**< Soglia di utenti in attesa che causa explode (EXPLODE_THRESHOLD) */
    int   ns_per_minuto;       /**< Nanosecondi reali per simulare 1 minuto (N_NANO_SECS) */
    int   durata_sim;          /**< Numero totale di giorni da simulare (SIM_DURATION) */
    float p_min;               /**< Probabilità minima che un utente vada in posta (P_SERV_MIN) */
    float p_max;               /**< Probabilità massima che un utente vada in posta (P_SERV_MAX) */
    int   max_pause;           /**< Numero massimo di pause per operatore nell'intera simulazione (NOF_PAUSE) */
    int   num_richieste;       /**< Numero massimo di richieste per utente al giorno (N_REQUESTS) */
} Config;

/**
 * @brief Carica la configurazione dal file specificato.
 * 
 * Legge il file di configurazione in formato chiave=valore e popola i parametri
 * passati come puntatori. Se il file non esiste o non può essere letto,
 * vengono utilizzati valori predefiniti hardcoded.
 * 
 * Formato del file config.conf:
 * - NOF_WORKERS=<numero>        : Numero di operatori
 * - NOF_USERS=<numero>          : Numero di utenti iniziali
 * - NOF_WORKER_SEATS=<numero>   : Numero di sportelli
 * - SIM_DURATION=<numero>       : Giorni da simulare
 * - N_NANO_SECS=<numero>        : Nanosecondi per minuto simulato
 * - NOF_PAUSE=<numero>          : Pause massime per operatore
 * - P_SERV_MIN=<float>          : Probabilità minima (0.0-1.0)
 * - P_SERV_MAX=<float>          : Probabilità massima (0.0-1.0)
 * - EXPLODE_THRESHOLD=<numero>  : Soglia explode
 * - DAY_DURATION=<numero>       : Minuti per giorno
 * - N_REQUESTS=<numero>         : Richieste max per utente
 * 
 * @param percorso Percorso del file di configurazione (può essere NULL per usare il predefinito)
 * @param num_operatori Puntatore dove memorizzare il numero di operatori (NOF_WORKERS)
 * @param num_utenti Puntatore dove memorizzare il numero di utenti (NOF_USERS)
 * @param num_sportelli Puntatore dove memorizzare il numero di sportelli (NOF_WORKER_SEATS)
 * @param durata_sim Puntatore dove memorizzare la durata della simulazione (SIM_DURATION)
 * @param ns_per_minuto Puntatore dove memorizzare i nanosecondi per minuto (N_NANO_SECS)
 * @param max_pause Puntatore dove memorizzare il numero massimo di pause (NOF_PAUSE)
 * @param p_min Puntatore dove memorizzare la probabilità minima (P_SERV_MIN)
 * @param p_max Puntatore dove memorizzare la probabilità massima (P_SERV_MAX)
 * @param soglia_attesa Puntatore dove memorizzare la soglia explode (EXPLODE_THRESHOLD)
 * @param durata_giorno Puntatore dove memorizzare la durata del giorno (DAY_DURATION)
 * @param num_richieste Puntatore dove memorizzare il numero di richieste (N_REQUESTS)
 * 
 * @note Tutti i puntatori possono essere NULL se il parametro non è necessario.
 * @note I valori predefiniti vengono usati se il file non esiste o se una chiave non è presente.
 */
void carica_configurazione(const char* percorso,
                           int* num_operatori, int* num_utenti, int* num_sportelli,
                           int* durata_sim, int* ns_per_minuto, int* max_pause,
                           float* p_min, float* p_max, int* soglia_attesa, int* durata_giorno,
                           int* num_richieste);

/**
 * @brief Risolve il percorso del file di configurazione da utilizzare.
 * 
 * Determina quale file di configurazione utilizzare seguendo questa priorità:
 * 1. Variabile d'ambiente POSTE_CONFIG (se impostata e non vuota)
 * 2. Argomento da riga di comando --config=percorso
 * 3. Valore predefinito PERCORSO_FILE_CONFIG ("config.conf")
 * 
 * @param argc Numero di argomenti della riga di comando
 * @param argv Array degli argomenti della riga di comando
 * @return Puntatore costante alla stringa contenente il percorso da utilizzare
 * 
 * @note Il puntatore restituito può puntare a memoria statica o agli argomenti del programma.
 * @note Non è necessario liberare la memoria restituita.
 * 
 * @example
 * // Uso da riga di comando:
 * // ./bin/direttore --config=config_explode.conf
 * // O con variabile d'ambiente:
 * // export POSTE_CONFIG=config_custom.conf && ./bin/direttore
 */
const char* risolvi_percorso_config(int argc, char** argv);

#endif /* CONFIG_H */
