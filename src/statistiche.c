#include "statistiche.h"
#include "config.h"
#include "semafori.h"
#include "utilita.h" 
#include <stdio.h>

// Struttura interna per i calcoli
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

static CalcoliStatistici calcola_statistiche_interne(SharedMemory* shm) {
    CalcoliStatistici calc;
    
    float giorni_trascorsi = (shm->giorno > 0) ? (float)shm->giorno : 1.0f;
    
    calc.servizi_tot = 0;
    calc.nonserv_tot = 0;
    for (int i = 0; i < NUM_SERVIZI; i++) {
        calc.servizi_tot += shm->totale_servizi_completati[i];
        calc.nonserv_tot += shm->totale_servizi_non_completati[i];
    }

    calc.media_servizi_giorno = (float)calc.servizi_tot / giorni_trascorsi;
    calc.media_nonserv_giorno = (float)calc.nonserv_tot / giorni_trascorsi;
    calc.media_utenti_giorno = (float)shm->totale_utenti_serviti / giorni_trascorsi;
    calc.media_pause_giorno = (float)shm->pause_totali / giorni_trascorsi;

    // Medie Simulazione
    calc.avg_att_sim = (shm->totale_utenti_serviti > 0) ?
        (double)shm->somma_attese_totali / shm->totale_utenti_serviti : 0.0;
        
    calc.avg_srv_sim = (shm->totale_utenti_serviti > 0) ?
        (double)shm->somma_durata_servizi_totali / shm->totale_utenti_serviti : 0.0;
    
    // Medie Giornaliere
    calc.avg_att_giorno = (shm->utenti_serviti_oggi > 0) ?
        (double)shm->somma_attese_oggi / shm->utenti_serviti_oggi : 0.0;
        
    int servizi_oggi = 0;
    for(int i=0; i<NUM_SERVIZI; i++) servizi_oggi += shm->servizi_completati_oggi[i];
    
    calc.avg_srv_giorno = (servizi_oggi > 0) ?
        (double)shm->somma_durata_servizi_oggi / servizi_oggi : 0.0;
    
    return calc;
}

void inizializza_file_csv(void) {
    FILE* file = fopen("statistiche.csv", "w");
    if (!file) {
        perror("[Statistiche] Impossibile creare statistiche.csv");
        return;
    }
    
    fprintf(file, "Giorno,Tipo_Statistica,");
    fprintf(file, "Utenti_Serviti_Tot,Utenti_Media_Giorno,");
    fprintf(file, "Servizi_Tot,Servizi_Fail_Tot,");
    fprintf(file, "Servizi_Media_Giorno,Servizi_Fail_Media_Giorno,");
    fprintf(file, "Attesa_Media_Sim,Attesa_Media_Oggi,");
    fprintf(file, "Durata_Media_Sim,Durata_Media_Oggi,");
    fprintf(file, "Ops_Attivi_Oggi,Ops_Attivi_Sim,");
    fprintf(file, "Pause_Media_Giorno,Pause_Tot");

    for (int i = 1; i <= NUM_SERVIZI; i++) {
        fprintf(file, ",S%d_Ok,S%d_Fail,S%d_Ratio", i, i, i);
    }
    fprintf(file, "\n");
    fclose(file);
}

static void scrivi_csv(SharedMemory* shm, CalcoliStatistici* calc, int is_final) {
    FILE* file = fopen("statistiche.csv", "a");
    if (!file) return;

    // Determina etichetta finale
    const char* label = "GIORNALIERA";
    if (is_final) {
        if (shm->motivo_termine == 2) label = "FINALE_EXPLODE";
        else label = "FINALE_NATURALE";
    }

    fprintf(file, "%d,%s,%d,%.2f,%d,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%d,%.2f,%d",
            shm->giorno,
            label,
            shm->totale_utenti_serviti,
            calc->media_utenti_giorno,
            calc->servizi_tot,
            calc->nonserv_tot,
            calc->media_servizi_giorno,
            calc->media_nonserv_giorno,
            calc->avg_att_sim,
            calc->avg_att_giorno,
            calc->avg_srv_sim,
            calc->avg_srv_giorno,
            shm->operatori_attivi_oggi,
            shm->operatori_attivi_sim,
            calc->media_pause_giorno,
            shm->pause_totali);

    for (int i = 0; i < NUM_SERVIZI; i++) {
        double rapporto = -1.0;
        // Calcola rapporto solo se non è finale (o come preferisci)
        if (!is_final) {
            int ops = shm->operatori_per_servizio[i];
            int spt = shm->sportelli_per_servizio_oggi[i];
            if (spt > 0) rapporto = (double)ops / (double)spt;
        }
        
        fprintf(file, ",%d,%d,%.2f",
                shm->totale_servizi_completati[i],
                shm->totale_servizi_non_completati[i],
                rapporto);
    }
    
    fprintf(file, "\n");
    fclose(file);
}

void stampa_statistiche(SharedMemory* shm, int sem_id, int is_final) {
    // 1. Acquisizione Lock
    sem_wait_operation(sem_id, SEM_MUTEX, 1);

    // 2. Calcoli
    CalcoliStatistici calc = calcola_statistiche_interne(shm);
    int motivo = shm->motivo_termine;

    // 3. Stampa a Video (Formattazione richiesta)
    printf("\n=====================================================\n");
    if (is_final) {
        if (motivo == 2) {
            printf("STATISTICHE FINALI - Simulazione terminata dopo %d giorni, causa: explode\n", shm->giorno);
        } else if (motivo == 1) {
            printf("STATISTICHE FINALI - Simulazione terminata dopo %d giorni, causa: fine simulazione\n", shm->giorno);
        } else {
            // Caso 3: Interruzione manuale (CTRL+C)
            printf("STATISTICHE FINALI - Simulazione interrotta manualmente al giorno %d\n", shm->giorno);
        }
    } else {
        printf("STATISTICHE DEL GIORNO %d\n", shm->giorno);
    }
    printf("=====================================================\n");

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

        if (is_final) {
             printf("  - Rapporto operatori/sportelli: N/A (Fine Sim)\n");
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

    // 4. Scrittura CSV
    scrivi_csv(shm, &calc, is_final);

    // 5. Rilascio Lock
    sem_post_operation(sem_id, SEM_MUTEX, 1);
}