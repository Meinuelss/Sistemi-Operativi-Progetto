#include "statistiche.h"
#include "config.h"
#include <stdio.h>

typedef struct {
    int servizi_tot;
    int nonserv_tot;
    float media_servizi_giorno;
    float media_nonserv_giorno;
    float media_utenti_giorno;
    float media_pause_giorno;
    double avg_att_sim;
    double avg_srv_sim;
    double avg_att_giorno;
    double avg_srv_giorno;
} CalcoliStatistici;

static CalcoliStatistici calcola_statistiche(SharedMemory* shm) {
    CalcoliStatistici calc;
    float denom_giorni = (shm->giorno > 0) ? (float)shm->giorno : 1.0f;
    
    calc.servizi_tot = 0;
    calc.nonserv_tot = 0;
    for (int i = 0; i < NUM_SERVIZI; i++) {
        calc.servizi_tot += shm->totale_servizi_completati[i];
        calc.nonserv_tot += shm->totale_servizi_non_completati[i];
    }

    calc.media_servizi_giorno = (float)calc.servizi_tot / denom_giorni;
    calc.media_nonserv_giorno = (float)calc.nonserv_tot / denom_giorni;
    calc.media_utenti_giorno = (float)shm->totale_utenti_serviti / denom_giorni;
    calc.media_pause_giorno = (float)shm->pause_totali / denom_giorni;

    calc.avg_att_sim = (shm->totale_utenti_serviti > 0) ?
        (double)shm->somma_attese_totali / shm->totale_utenti_serviti : 0.0;
    calc.avg_srv_sim = (shm->totale_utenti_serviti > 0) ?
        (double)shm->somma_durata_servizi_totali / shm->totale_utenti_serviti : 0.0;
    
    calc.avg_att_giorno = (shm->utenti_serviti_oggi > 0) ?
        (double)shm->somma_attese_oggi / shm->utenti_serviti_oggi : 0.0;
    calc.avg_srv_giorno = (shm->utenti_serviti_oggi > 0) ?
        (double)shm->somma_durata_servizi_oggi / shm->utenti_serviti_oggi : 0.0;
    
    return calc;
}

void inizializza_file_csv(void) {
    FILE* file = fopen("statistiche.csv", "w");
    if (!file) return;
    
    fprintf(file, "Giorno,Tipo_Statistica,");
    fprintf(file, "Utenti_Serviti_Totali,Utenti_Serviti_Media_Giorno,");
    fprintf(file, "Servizi_Erogati_Totali,Servizi_Non_Erogati_Totali,");
    fprintf(file, "Servizi_Erogati_Media_Giorno,Servizi_Non_Erogati_Media_Giorno,");
    fprintf(file, "Tempo_Medio_Attesa_Simulazione,Tempo_Medio_Attesa_Giorno,");
    fprintf(file, "Tempo_Medio_Erogazione_Simulazione,Tempo_Medio_Erogazione_Giorno,");
    fprintf(file, "Operatori_Attivi_Giorno,Operatori_Attivi_Simulazione,");
    fprintf(file, "Pause_Media_Giorno,Pause_Totali_Simulazione");

    for (int i = 1; i <= NUM_SERVIZI; i++) {
        fprintf(file, ",Servizio_%d_Completati,Servizio_%d_Non_Completati,Servizio_%d_Rapporto_Op_Sport",
                i, i, i);
    }
    fprintf(file, "\n");
    
    fclose(file);
}


void aggiungi_statistiche_a_csv(SharedMemory* shm, int is_final) {
    FILE* file = fopen("statistiche.csv", "a");
    if (!file) return;

    CalcoliStatistici calc = calcola_statistiche(shm);

    fprintf(file, "%d,%s,%d,%.2f,%d,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%d,%.2f,%d",
            shm->giorno,
            is_final ? "FINALE" : "GIORNALIERA",
            shm->totale_utenti_serviti,
            calc.media_utenti_giorno,
            calc.servizi_tot,
            calc.nonserv_tot,
            calc.media_servizi_giorno,
            calc.media_nonserv_giorno,
            calc.avg_att_sim,
            calc.avg_att_giorno,
            calc.avg_srv_sim,
            calc.avg_srv_giorno,
            shm->operatori_attivi_oggi,
            shm->operatori_attivi_sim,
            calc.media_pause_giorno,
            shm->pause_totali);

    for (int i = 0; i < NUM_SERVIZI; i++) {
        double rapporto;
        if(is_final == 1){
            rapporto = 0.0;
        }else{
            int ops = shm->operatori_per_servizio[i];
            int spt = shm->sportelli_per_servizio_oggi[i];
            rapporto = (spt > 0) ? (double)ops / (double)spt : -1.0;
        }
        fprintf(file, ",%d,%d,%.2f",
                shm->totale_servizi_completati[i],
                shm->totale_servizi_non_completati[i],
                rapporto);
    }
    
    fprintf(file, "\n");
    fclose(file);
}

void stampa_statistiche(SharedMemory* shm, int is_final, int motivo_termine) {
    printf("\n=====================================================\n");
    if (is_final && motivo_termine == 2) {
        printf("STATISTICHE FINALI - Simulazione terminata dopo %d giorni, causa: explode\n", shm->giorno);
    } else if (is_final && motivo_termine == 1)
    {
        printf("STATISTICHE FINALI - Simulazione terminata dopo %d giorni, causa: fine simulazione\n", shm->giorno);
    }else{
        printf("STATISTICHE DEL GIORNO %d\n", shm->giorno);
    }
    printf("=====================================================\n");

    CalcoliStatistici calc = calcola_statistiche(shm);

    printf("- numero di utenti serviti totali nella simulazione: %d\n", shm->totale_utenti_serviti);
    printf("- numero di utenti serviti in media al giorno: %.2f\n", calc.media_utenti_giorno);
    printf("- numero di servizi erogati totali nella simulazione: %d\n", calc.servizi_tot);
    printf("- numero di servizi non erogati totali nella simulazione: %d\n", calc.nonserv_tot);
    printf("- numero di servizi erogati in media al giorno: %.2f\n", calc.media_servizi_giorno);
    printf("- numero di servizi non erogati in media al giorno: %.2f\n", calc.media_nonserv_giorno);
    printf("- tempo medio di attesa degli utenti nella simulazione: %.2f\n", calc.avg_att_sim);
    printf("- tempo medio di attesa degli utenti nella giornata: %.2f\n", calc.avg_att_giorno);
    printf("- tempo medio di erogazione dei servizi nella simulazione: %.2f\n", calc.avg_srv_sim);
    printf("- tempo medio di erogazione dei servizi nella giornata: %.2f\n", calc.avg_srv_giorno);

    static const char* nomi_servizi[NUM_SERVIZI] = {
        "Invio e ritiro pacchi",
        "Invio e ritiro lettere raccomandate",
        "Prelievi e pagamenti bancoposta",
        "Pagamento bollettini postali",
        "Acquisto prodotti finanziari",
        "Acquisto orologi e braccialetti"
    };
    
    printf("\n----- Statistiche per Tipo di Servizio -----\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        printf("\nServizio: %s\n", nomi_servizi[i]);
        printf("  - Servizi completati: %d\n", shm->totale_servizi_completati[i]);
        printf("  - Servizi non completati: %d\n", shm->totale_servizi_non_completati[i]);
        
        int ops = shm->operatori_per_servizio[i];
        int spt = shm->sportelli_per_servizio_oggi[i];

        if(is_final == 1){
            printf("  - Rapporto operatori/sportelli: 0/0 (0)\n");
        } else if (spt > 0) {
            printf("  - Rapporto operatori/sportelli: %d/%d (%.2f)\n", ops, spt, (double)ops / (double)spt);
        } else {
            printf("  - Rapporto operatori/sportelli: %d/%d (-1.00)\n", ops, spt);
        }


    }

    printf("\n----- Statistiche Operatori e Pause -----\n");
    printf("- numero di operatori attivi durante la giornata: %d\n", shm->operatori_attivi_oggi);
    printf("- numero di operatori attivi durante la simulazione: %d\n", shm->operatori_attivi_sim);
    printf("- numero medio di pause effettuate nella giornata: %.2f\n", calc.media_pause_giorno);
    printf("- totale di pause effettuate durante la simulazione: %d\n", shm->pause_totali);

    printf("=====================================================\n\n");
    aggiungi_statistiche_a_csv(shm, is_final);
}
