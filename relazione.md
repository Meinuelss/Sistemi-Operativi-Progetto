
# Relazione Progetto SO Riccioni Serranò

## Processo Direttore
All'avvio il direttore crea e inizializza tutte le risorse necessarie alla simulazione: la memoria condivisa, i semafori e la coda messaggi; quindi genera i processi figli richiesti un unico erogatore_ticket, NOF_WORKER_SEATS sportelli, NOF_WORKERS processi operatore e NOF_USERS processi utente.
Dopo la creazione, il direttore attende una barriera di sincronizzazione: la simulazione inizia solo quando l'erogatore, tutti gli operatori e tutti gli utenti hanno completato la fase di inizializzazione e segnalato la loro disponibilità.
Il direttore governa il tempo simulato con un ciclo principale di SIM_DURATION giorni; ogni giorno è suddiviso in minuti simulati e il passaggio di un minuto viene ottenuto mediante una breve pausa reale di N_NANO_SECS nanosecondi (nanosleep). Il tempo corrente è aggiornato in memoria condivisa così che i processi figli possano sincronizzarsi leggendo lo stato globale.
Al termine di ogni giornata il direttore raccoglie le statistiche finali di giornata dalla memoria condivisa (le statistiche sono aggiornate dagli attori della simulazione), le stampa e le registra su file; alla fine della simulazione stampa il report complessivo.
La terminazione può avvenire per raggiungimento della durata configurata o per condizioni di EXPLODE. In entrambi i casi il direttore notifica i figli, attende la loro terminazione con waitpid, pulisce le risorse IPC e termina.

## Processo Erogatore Ticket

All'avvio si connette alla memoria condivisa, ai semafori e alla coda messaggi, imposta il flag erogatore_inizializzato e segnala la barriera di inizializzazione (SEM_INIT_BARRIER).
Riceve le richieste degli utenti tramite la coda di messaggi (messaggi di tipo TIPO_MSG_RICHIESTA contenenti pid_utente e tipo_servizio).
Per ogni richiesta valida: legge da memoria condivisa il giorno corrente e il numero di sportelli disponibili per il servizio richiesto se l'ufficio è aperto, se il giorno è chiuso o il servizio non è disponibile invia una risposta con ESITO_NON_SERVITO_GIORNO al PID dell'utente, se non ci sono sportelli disponibili incrementa i conteggi di non servito e invia ESITO_NON_SERVITO_GIORNO, se ci sono sportelli disponibili prova ad accodare l'utente nella coda del servizio (struttura shm->code_servizio[indice]), se l'accodamento riesce segnala gli operatori tramite il semaforo corrispondente (SEM_BASE_SERVIZI + indice) e aggiorna utenti_in_attesa, se la coda è piena marca la richiesta come non servita e invia ESITO_NON_SERVITO_GIORNO.
L'erogatore NON esegue il servizio: si limita a smistare le richieste verso le code dei servizi. Gli operatori poi prelevano dalle code e inviano la risposta finale (ESITO_SERVITO) all'utente.
Alla terminazione pulisce la connessione alla memoria condivisa e termina.

## Risorse Sportello

Ogni giorno il direttore assegna a ciascun sportello un tipo di servizio, è possibile che più sportelli offrano lo stesso servizio, o che alcuni servizi non siano offerti quel giorno.
La mappatura sportello->servizio è mantenuta in memoria condivisa nell'array sportelli_per_servizio_oggi e viene letta sia dall'erogatore che dagli utenti per sapere quali servizi sono disponibili.
Ogni sportello può ospitare al massimo un operatore: la politica di associazione operatore <--> sportello da noi applicata si basa sul tipo di servizio associato ai singoli operatori, quando un operatore cerca uno sportello per il suo servizio, scorre un array, l'array sportelli, fino a quando non trova uno sportello libero e che sia associato al suo stesso servizio. 
Durante la giornata gli operatori che risultano associati a uno sportello gestiscono la coda del servizio corrispondente, se un operatore va in pausa termina la giornata anticipatamente lasciando libero lo sportello per una possibile sostituzione.

## Processo Operatore

Ogni operatore ha assegnato un servizio fisso calcolato dall'id dato da mio_servizio = (id-1) % NUM_SERVIZI + 1 che resta invariato per tutta la simulazione.
Compete per lo sportello all'inizio di ogni giornata, cerca uno sportello libero che offra il suo servizio; la ricerca e l'acquisizione sono gestite da acquisisci_sportello che setta sportelli[indice].pid_operatore quando trova uno slot libero.
Se non trova uno sportello resta in attesa sul semaforo di sostituzione SEM_SOSTITUZIONE_BASE + servizio - 1 finché uno sportello non si libera; se a fine giornata non ha ottenuto uno sportello torna a casa e si ripresenta il giorno successivo.
Quando invece occupa uno sportello l'operatore estrae utenti dalla coda del suo servizio estrai_da_coda, sincronizzando con SEM_BASE_SERVIZI + indice e proteggendo gli accessi con SEM_MUTEX. Per ogni utente calcola la durata del servizio basata su minuti_base_servizio con componente casuale, dorme il corrispondente numero di minuti simulati e aggiorna le statistiche condivise quindin attese, durata, servizi completati e utenti distinti serviti. Invia la risposta all'utente ESITO_SERVITO o ESITO_NON_SERVITO_GIORNO se il giorno si è chiuso.
L'operatore può effettuare fino a max_pause pause nell'intera simulazione, quando prende una pausa libera lo sportello, incrementa i contatori di pausa giornalieri e totali e notifica la possibilità di sostituzione tramite semaforo.
Alla chiusura libera lo sportello (se occupato), incrementa operatori_fine_giorno, segnala SEM_OPERATORI_FINE_GIORNO e attende la fine della giornata.
Alla terminazione pulisce la connessione alla memoria condivisa e termina.

## Processo Utente

Ogni utente ha una probabilità personale p_serv scelta all'avvio nell'intervallo [p_min, p_max] da shm->config. Ogni giorno decide indipendentemente se andare all'ufficio in base a p_serv.
Se va in ufficio sceglie un tipo di servizio tra i 6 servizi offerti dall'ufficio e un orario di arrivo, minuto casuale entro la giornata. Si reca all'ufficio, dormendo fino al minuto scelto dormi_per_minuti.
Controlla se quel servizio è offerto quel giorno leggendo sportelli_per_servizio_oggi, se sì, crea un array di richieste e invia dei MessaggioRichiesta sulla coda di messaggi TIPO_MSG_RICHIESTA con pid_utente e tipo_servizio.
Dopo l'invio attende la MessaggioRisposta indirizzata al suo PID. Se la risposta contiene ESITO_SERVITO il servizio è stato erogato, se ESITO_NON_SERVITO_GIORNO l'utente è informato che non verrà servito.
Se al termine della giornata l'utente è ancora in coda quindi non ha ricevuto nessuna risposta abbandona l'ufficio e il sistema conta questo come servizio non erogato.

## Terminazione

La simulazione termina in due casi distinti come implementato in direttore.c: Timeout normale se si raggiunge la durata impostata SIM_DURATION (numero di giorni). Il direttore completa il ciclo dei giorni e la simulazione termina con motivo fine simulazione.
EXPLODE se al termine di una giornata il numero di utenti in attesa utenti_in_attesa supera la soglia configurata EXPLODE_THRESHOLD nel codice shm->config.soglia_attesa, il direttore imposta motivo_termine = 2 e termina la simulazione per causa EXPLODE.
Utilizziamo due file config: config.conf, file di configurazione per verificare la terminazione per timeout e config_explode.conf file di configurazione che imposti EXPLODE_THRESHOLD basso e/o parametri numero utenti/operatori, code, p_serv tali da generare l'accumulo di utenti in coda oltre la soglia.
Il programma stampa la causa della terminazione fine simulazione o explode insieme alle statistiche finali. Nel codice la variabile motivo_termine è passata a stampa_statistiche e causa la stampa di una riga descrittiva differente per motivo_termine == 2.

## Versione Completa (MAX 30)

Per quanto riguarda l'aggiunta della possibilità da parte dell'utente di effettuare più richieste, l'implementazione è gia stata affrontata e descritta nella descrizione dell'implementazione del processo utente.

Per l'aggiunta di ulteriori utenti durante la simulazione si invoca da riga di comando:
./bin/aggiungi_utenti <numero_nuovi_utenti>.
Il file aggiungi_utenti.c tenta di collegarsi alla memoria condivisa della simulazione con prova_ottieni_memoria_condivisa(), con un breve retry se la risorsa non è immediatamente disponibile, se non trova la simulazione dopo il tentativo iniziale il programma esce con messaggio di errore se la simulazione è avviata correttamente registra i nuovi PID e lascia che i processi utente partecipino alla simulazione come quelli generati dal direttore.
Carica la configurazione per mantenere coerenza con la simulazione usando carica_configurazione e per ogni nuovo utente richiesto esegue fork() e execv (UTENTE_EXEC, ...) per avviare un nuovo processo utente.
Sotto protezione del semaforo SEM_MUTEX registra il PID appena creato in shm->utenti_dinamici e incrementa shm->num_utenti_dinamici.
L'aggiunta è dinamica: i nuovi utenti si sincronizzano con le giornate correnti grazie ai semafori (SEM_DAY_START, SEM_DAY_END) e leggono shm->config per comportarsi coerentemente e partecipano alle giornate correnti grazie ai semafori; il direttore li considera nel conteggio di processi attivi usando shm->num_utenti_dinamici.
Infine stampa log informativi per ogni utente creato e poi si disconnette dalla memoria condivisa.

Per quanto riguarda il salvataggio delle statistiche in CSV il sistema crea e aggiorna un file statistiche.csv nella root del progetto.
Utilizza funzioni come: inizializza_file_csv() che crea statistiche.csv e scrive l'intestazione con tutte le colonne, aggiungi_statistiche_a_csv(SharedMemory* shm, int is_final) che appende una riga giornaliera o finale con i calcoli statistici.
Le scritture in statistiche.csv sono eseguite dal direttore tramite le chiamate a aggiungi_statistiche_a_csv e stampa_statistiche, dopo che le statistiche giornaliere sono raccolte e finalizzate in memoria condivisa.


## Configurazione a tempo di esecuzione
Tutti i parametri di configurazione sono letti a tempo di esecuzione dal file di configurazione selezionato o se non presente nessun file di configurazione userà i valori di default presi da config.c.
Quindi, non è necessaria e non deve essere effettuata alcuna ricompilazione del codice quando si modifica una configurazione: basta editare il file di configurazione appropriato prima di avviare la simulazione;
Non è consentito inserire o modificare singoli parametri da terminale una volta che la simulazione è in esecuzione: i processi leggono la configurazione all'avvio e la mantengono in memoria condivisa per tutta la simulazione; cambiamenti dinamici per singoli parametri non sono previsti dal progetto.

## Requisiti implementativi

Evitare l'attesa attiva: il progetto usa semafori System V e code di messaggi per la sincronizzazione e la comunicazione; i processi si bloccano su operazioni di attesa (sem_wait/msgrcv) invece di eseguire busy-wait.
Uso richiesto di IPC: nel progetto vengono usati memoria condivisa per contatori e strutture comuni, semafori per sincronizzazione e code di messaggi per richieste/risposte.
Modularità e divisione in moduli: ogni processo è un eseguibile separato, e il codice è organizzato in file separati (.c/.h).
Compilazione con Makefile: il repository include un Makefile per la build, e compila con le opzioni consigliate gcc -Wvla -Wextra -Werror -D_GNU_SOURCE.
Massimizzare il grado di concorrenza: architettura basata su processi indipendenti che interagiscono via IPC.
Deallocazione risorse IPC: al termine direttore rimuove la coda di messaggi, i semafori e la memoria condivisa tramite le funzioni rimuovi_coda_messaggi, rimuovi_semafori, rimuovi_memoria_condivisa.
Esecuzione su macchine che presentano parallelismo: il progetto è stato testato su una VM Linux.






