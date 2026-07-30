/**
 * @file servizi.h
 * @brief Definizione del tempario dei servizi postali.
 * 
 * Questo modulo fornisce i tempi base di erogazione per ciascuna tipologia di servizio.
 * I tempi effettivi vengono randomizzati nell'intervallo [base*0.5, base*1.5] per simulare
 * la variabilità reale del tempo di servizio.
 */

#ifndef SERVIZI_H
#define SERVIZI_H

#include "config.h"

/**
 * @brief Restituisce il tempo base di erogazione per un tipo di servizio.
 * 
 * Ogni servizio ha un tempo base predefinito in minuti simulati.
 * Il tempario corrente è:
 * - Servizio 1 (Invio/ritiro pacchi):               10 minuti
 * - Servizio 2 (Invio/ritiro raccomandate):          8 minuti
 * - Servizio 3 (Prelievi/pagamenti bancoposta):      6 minuti
 * - Servizio 4 (Pagamento bollettini):               8 minuti
 * - Servizio 5 (Prodotti finanziari):               20 minuti
 * - Servizio 6 (Orologi e braccialetti):            20 minuti
 * 
 * Il tempo effettivo di servizio viene calcolato come:
 *   tempo_effettivo = round(tempo_base * (0.5 + random(0,1)))
 * 
 * Questo produce una distribuzione uniforme nell'intervallo [base*0.5, base*1.5],
 * simulando la variabilità naturale dei tempi di servizio.
 * 
 * @param tipo_servizio Tipo di servizio richiesto (1-based, range 1..NUM_SERVIZI)
 * @return Tempo base in minuti simulati per il servizio specificato
 * 
 * @note Se tipo_servizio è fuori range, viene automaticamente normalizzato al range valido
 * @note La funzione è inline per prestazioni ottimali
 * @note I valori sono statici e condivisi tra tutte le invocazioni
 * 
 * @example
 * int tempo_base = minuti_base_servizio(1);  // Restituisce 10
 * int variazione = rand() % 1000 / 1000.0;   // 0.0..1.0
 * int tempo_reale = round(tempo_base * (0.5 + variazione));
 */
static inline int minuti_base_servizio(int tipo_servizio) {
    static const int base_predefinita[NUM_SERVIZI] = {10,8,6,8,20,20};
    if (tipo_servizio < 1) tipo_servizio = 1;
    if (tipo_servizio > NUM_SERVIZI) tipo_servizio = NUM_SERVIZI;
    return base_predefinita[tipo_servizio-1];
}

#endif /* SERVIZI_H */
