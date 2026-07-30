#include "coda.h"

void inizializza_coda(Coda* coda) {
    coda->testa = 0;
    coda->coda = 0;
    coda->conteggio = 0;
}

int accoda(Coda* coda, ElementoCoda valore) {
    if (coda_piena(coda)) return -1;
    coda->elementi[coda->coda] = valore;
    coda->coda = (coda->coda + 1) % LUNGHEZZA_MAX_CODA;
    coda->conteggio++;
    return 0;
}

int estrai_da_coda(Coda* coda, ElementoCoda* valore) {
    if (coda_vuota(coda)) return -1;
    *valore = coda->elementi[coda->testa];
    coda->testa = (coda->testa + 1) % LUNGHEZZA_MAX_CODA;
    coda->conteggio--;
    return 0;
}

int coda_vuota(Coda* coda) { return coda->conteggio == 0; }

int coda_piena(Coda* coda) { return coda->conteggio == LUNGHEZZA_MAX_CODA; }
