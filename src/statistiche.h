/**
 * @file statistiche.h
 * @brief Gestione e reporting delle statistiche della simulazione.
 * 
 * Questo modulo fornisce funzioni per raccogliere, calcolare e presentare
 * le statistiche della simulazione dell'ufficio postale. Le statistiche
 * vengono raccolte sia giornalmente che cumulativamente per l'intera simulazione,
 * e vengono esportate in formato CSV e stampate a console.
 */

#ifndef STATISTICHE_H
#define STATISTICHE_H

#include "memoria_condivisa.h"

/**
 * @brief Stampa le statistiche della simulazione su console.
 * 
 * Genera un report completo delle statistiche, formattato per la lettura umana.
 * Il report include:
 * 
 * Statistiche Globali:
 * - Numero di utenti serviti totali e media giornaliera
 * - Numero di servizi erogati e non erogati (totali e medie giornaliere)
 * - Tempo medio di attesa (simulazione e giornata corrente)
 * - Tempo medio di erogazione servizi (simulazione e giornata corrente)
 * 
 * Statistiche per Tipo di Servizio:
 * - Servizi completati e non completati per ciascun tipo
 * - Rapporto operatori/sportelli per servizio (solo statistiche giornaliere)
 * - Nomi descrittivi dei servizi (pacchi, raccomandate, ecc.)
 * 
 * Statistiche Operatori:
 * - Operatori attivi nella giornata
 * - Operatori attivi totali nella simulazione
 * - Numero medio di pause per giornata
 * - Totale pause nella simulazione
 * 
 * Motivi di terminazione:
 * - Fine simulazione (raggiunto SIM_DURATION)
 * - Explode (troppi utenti in attesa)
 * 
 * @param shm Puntatore alla memoria condivisa contenente le statistiche
 * @param is_final 0 per statistiche giornaliere, 1 per statistiche finali
 * @param motivo_termine Motivo della terminazione: 0=in corso, 1=fine simulazione, 2=explode
 * 
 * @pre shm != NULL
 * @post Le statistiche vengono stampate su stdout
 * @post Viene chiamata automaticamente aggiungi_statistiche_a_csv
 * 
 * @note Le statistiche giornaliere mostrano il rapporto operatori/sportelli
 * @note Le statistiche finali impostano il rapporto a 0/0 (servizio terminato)
 */
void stampa_statistiche(SharedMemory* shm, int is_final, int motivo_termine);

/**
 * @brief Appende una riga di statistiche al file CSV.
 * 
 * Scrive una riga nel file statistiche.csv con tutti i dati statistici
 * in formato machine-readable. Il CSV contiene:
 * 
 * Colonne principali:
 * - Giorno: numero del giorno (1-based)
 * - Tipo_Statistica: "GIORNALIERA" o "FINALE"
 * - Utenti_Serviti_Totali, Utenti_Serviti_Media_Giorno
 * - Servizi_Erogati_Totali, Servizi_Non_Erogati_Totali
 * - Servizi_Erogati_Media_Giorno, Servizi_Non_Erogati_Media_Giorno
 * - Tempo_Medio_Attesa_Simulazione, Tempo_Medio_Attesa_Giorno
 * - Tempo_Medio_Erogazione_Simulazione, Tempo_Medio_Erogazione_Giorno
 * - Operatori_Attivi_Giorno, Operatori_Attivi_Simulazione
 * - Pause_Media_Giorno, Pause_Totali_Simulazione
 * 
 * Per ogni servizio (1..6):
 * - Servizio_N_Completati
 * - Servizio_N_Non_Completati
 * - Servizio_N_Rapporto_Op_Sport
 * 
 * Formato numeri:
 * - Interi per conteggi
 * - Float con 2 decimali per medie e rapporti
 * - -1.00 per rapporti non definiti (0 sportelli)
 * 
 * @param shm Puntatore alla memoria condivisa contenente le statistiche
 * @param is_final 0 per riga giornaliera, 1 per riga finale
 * 
 * @pre shm != NULL
 * @pre Il file statistiche.csv deve essere stato creato con inizializza_file_csv
 * @post Una nuova riga viene aggiunta al file CSV
 * @post Il file viene chiuso dopo la scrittura (flush automatico)
 * 
 * @note Apre il file in modalità append ("a")
 * @note Se l'apertura fallisce, ritorna silenziosamente senza scrivere
 */
void aggiungi_statistiche_a_csv(SharedMemory* shm, int is_final);

/**
 * @brief Inizializza il file CSV delle statistiche con l'intestazione.
 * 
 * Crea (o sovrascrive) il file statistiche.csv e scrive la riga di intestazione
 * con i nomi delle colonne. Deve essere chiamata all'inizio della simulazione,
 * prima di qualsiasi chiamata a aggiungi_statistiche_a_csv.
 * 
 * Il file viene creato nella directory corrente (dove viene eseguito il programma).
 * 
 * Intestazioni scritte:
 * - Colonne generali (giorno, tipo, utenti, servizi, tempi, operatori, pause)
 * - Colonne per-servizio (3 colonne per ogni servizio, totale 18 colonne)
 * 
 * Formato file:
 * - Separatore: virgola (,)
 * - Encoding: ASCII
 * - Terminatore riga: \n
 * - Prima riga: intestazioni
 * - Righe successive: dati (una per giorno + una finale)
 * 
 * @post Il file statistiche.csv esiste ed è vuoto eccetto l'intestazione
 * @post Il file è chiuso dopo la creazione
 * 
 * @note Se la creazione fallisce, ritorna silenziosamente senza errore
 * @note Se il file esiste, viene sovrascritto (modalità "w")
 * @note Deve essere chiamata SOLO dal processo direttore
 */
void inizializza_file_csv(void);

#endif /* STATISTICHE_H */
