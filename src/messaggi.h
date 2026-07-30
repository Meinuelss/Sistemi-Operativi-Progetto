/**
 * @file messaggi.h
 * @brief Sistema di messaggistica basato su code messaggi System V.
 * 
 * Implementa la comunicazione asincrona tra processi usando System V Message Queues.
 * La coda messaggi è utilizzata per:
 * 1. Utenti → Erogatore: richieste di servizio (tipo TIPO_MSG_RICHIESTA)
 * 2. Operatori → Utenti: risposte con esito del servizio (tipo = PID utente)
 * 
 * Architettura del routing:
 * - Tutti i messaggi di richiesta hanno mtype = TIPO_MSG_RICHIESTA (broadcast verso erogatore)
 * - Ogni risposta ha mtype = PID dell'utente destinatario (canale privato unicast)
 * - L'erogatore legge solo messaggi di tipo TIPO_MSG_RICHIESTA
 * - Ogni utente legge solo messaggi con mtype = proprio PID
 */

#ifndef MESSAGGI_H
#define MESSAGGI_H

#include <sys/types.h>

/**
 * @def TIPO_MSG_RICHIESTA
 * @brief Tipo di messaggio per le richieste di servizio da parte degli utenti.
 * 
 * Tutti i messaggi MessaggioRichiesta inviati dagli utenti usano questo
 * tipo, permettendo all'erogatore di filtrare e ricevere solo le richieste.
 */
#define TIPO_MSG_RICHIESTA 1

/**
 * @def MSG_FTOK_PERCORSO
 * @brief Percorso usato da ftok() per generare la chiave IPC della coda messaggi.
 */
#define MSG_FTOK_PERCORSO "/tmp"

/**
 * @def MSG_FTOK_PROGETTO
 * @brief Project ID usato da ftok() per generare la chiave IPC della coda messaggi.
 */
#define MSG_FTOK_PROGETTO 'M'

/**
 * @enum Codici di esito per le risposte ai servizi
 * @brief Definisce i possibili esiti di una richiesta di servizio.
 */
enum {
    ESITO_SERVITO = 0,           /**< Il servizio è stato completato con successo */
    ESITO_NON_SERVITO_GIORNO = 1 /**< Il servizio non può essere erogato oggi
                                       (giorno chiuso, coda piena, servizio non disponibile) */
};

/**
 * @struct MessaggioRichiesta
 * @brief Messaggio inviato dall'utente per richiedere un servizio.
 * 
 * Struttura del messaggio di richiesta:
 * - mtype: Identifica il tipo di messaggio (routing verso erogatore)
 * - pid_utente: PID del processo utente che invia la richiesta
 * - tipo_servizio: Servizio richiesto (1..NUM_SERVIZI)
 * 
 * Flusso:
 * 1. Utente crea MessaggioRichiesta
 * 2. Utente invia con msgsnd(msgid, &msg, size, 0)
 * 3. Erogatore riceve con msgrcv(msgid, &msg, size, TIPO_MSG_RICHIESTA, 0)
 * 4. Erogatore accoda la richiesta nella coda del servizio appropriato
 */
typedef struct {
    long mtype;             /**< Tipo del messaggio, sempre = TIPO_MSG_RICHIESTA */
    pid_t pid_utente;       /**< PID del processo utente richiedente (per la risposta) */
    int tipo_servizio;      /**< Tipo di servizio richiesto (1..NUM_SERVIZI) */
} MessaggioRichiesta;

/**
 * @struct MessaggioRisposta
 * @brief Messaggio di risposta inviato dal sistema all'utente.
 * 
 * Struttura del messaggio di risposta:
 * - mtype: PID dell'utente destinatario (routing unicast)
 * - esito: Risultato del servizio (ESITO_SERVITO o ESITO_NON_SERVITO_GIORNO)
 * - tipo_servizio: Servizio che è stato (o non è stato) erogato
 * - giorno: Giorno della simulazione in cui è stata processata la richiesta
 * 
 * Flusso:
 * 1. Operatore completa il servizio o identifica impossibilità di erogazione
 * 2. Sistema invia con invia_risposta(msgid, pid_utente, esito, servizio, giorno)
 * 3. Utente riceve con ricevi_risposta_per_pid(msgid, &risposta, 0)
 * 4. Utente verifica l'esito e agisce di conseguenza
 */
typedef struct {
    long mtype;             /**< Tipo del messaggio = PID dell'utente destinatario (canale privato) */
    int esito;              /**< Esito del servizio: ESITO_SERVITO o ESITO_NON_SERVITO_GIORNO */
    int tipo_servizio;      /**< Tipo di servizio erogato (o tentato) */
    int giorno;             /**< Giorno della simulazione in cui è stata processata la richiesta */
} MessaggioRisposta;

/**
 * @brief Crea la coda messaggi condivisa per l'IPC.
 * 
 * Crea una nuova coda messaggi System V o si connette a una esistente.
 * La chiave IPC è generata tramite ftok(MSG_FTOK_PERCORSO, MSG_FTOK_PROGETTO).
 * 
 * Usa IPC_CREAT (senza IPC_EXCL) per creare la coda se non esiste,
 * o riutilizzare quella esistente se già presente.
 * 
 * @return ID della coda messaggi creata/ottenuta, o -1 in caso di errore
 * 
 * @post La coda messaggi è accessibile tramite l'ID restituito
 * @note Stampa errore su stderr se ftok o msgget falliscono
 * @note Deve essere chiamata dal processo direttore all'avvio
 * @note Permessi: 0666 (lettura/scrittura per tutti)
 */
int crea_coda_messaggi(void);

/**
 * @brief Rimuove la coda messaggi dal sistema.
 * 
 * Elimina definitivamente la coda messaggi System V, liberando le risorse IPC.
 * Dopo questa chiamata, nessun processo può più inviare o ricevere messaggi.
 * 
 * @param msgid ID della coda messaggi da rimuovere
 * @return 0 in caso di successo, -1 in caso di errore
 * 
 * @pre msgid deve essere un ID valido ottenuto da crea_coda_messaggi
 * @post La coda messaggi non esiste più nel sistema
 * @note Stampa errore su stderr se msgctl fallisce
 * @note Deve essere chiamata dal direttore alla fine della simulazione
 */
int rimuovi_coda_messaggi(int msgid);

/**
 * @brief Invia una risposta a un utente specifico.
 * 
 * Crea e invia un MessaggioRisposta indirizzato a un utente specifico.
 * Il routing avviene tramite mtype = PID dell'utente destinatario,
 * garantendo che solo quell'utente riceverà il messaggio.
 * 
 * @param msgid ID della coda messaggi
 * @param pid_destinatario PID del processo utente a cui inviare la risposta
 * @param esito Esito del servizio (ESITO_SERVITO o ESITO_NON_SERVITO_GIORNO)
 * @param tipo_servizio Tipo di servizio erogato (1..NUM_SERVIZI)
 * @param giorno Giorno della simulazione corrente
 * @return 0 in caso di successo, -1 in caso di errore
 * 
 * @pre msgid valido, pid_destinatario > 0
 * @pre esito in {ESITO_SERVITO, ESITO_NON_SERVITO_GIORNO}
 * @pre tipo_servizio in range 1..NUM_SERVIZI
 * @post Il messaggio è nella coda, pronto per essere letto dall'utente
 * 
 * @note Usa msgsnd con flag 0 (bloccante se coda piena, ma improbabile)
 * @note Stampa errore su stderr se msgsnd fallisce
 */
int invia_risposta(int msgid, pid_t pid_destinatario, int esito, int tipo_servizio, int giorno);

/**
 * @brief Riceve una risposta indirizzata al PID del processo chiamante.
 * 
 * Legge un MessaggioRisposta dalla coda che ha mtype = getpid(),
 * garantendo che il processo riceva solo messaggi a lui destinati.
 * 
 * Modalità di ricezione (parametro flags):
 * - 0: bloccante (attende fino a che non arriva un messaggio)
 * - IPC_NOWAIT: non bloccante (ritorna subito con errore se nessun messaggio)
 * 
 * @param msgid ID della coda messaggi
 * @param out Puntatore a MessaggioRisposta dove memorizzare la risposta ricevuta
 * @param flags Flag di controllo (0 = bloccante, IPC_NOWAIT = non bloccante)
 * @return 0 in caso di successo, -1 in caso di errore
 * 
 * @pre msgid valido, out != NULL
 * @post Se successo, *out contiene il messaggio ricevuto
 * @post Il messaggio viene rimosso dalla coda
 * 
 * @note Usa msgrcv con mtype = getpid() (routing unicast)
 * @note Se flags = 0 e non ci sono messaggi, il processo si blocca
 * @note Se flags = IPC_NOWAIT e non ci sono messaggi, ritorna -1 con errno = ENOMSG
 * @note Può essere interrotta da segnali (ritorna -1 con errno = EINTR)
 */
int ricevi_risposta_per_pid(int msgid, MessaggioRisposta* out, int flags);

#endif /* MESSAGGI_H */
