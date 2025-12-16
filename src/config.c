#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

static int parse_line_kv(const char* line, char* key, size_t ksz, char* val, size_t vsz) {
    const char* eq = strchr(line, '=');
    if (!eq) return -1;
    size_t klen = (size_t)(eq - line);
    while (klen && (line[klen-1] == ' ' || line[klen-1] == '\t')) klen--;
    size_t vlen = strlen(eq + 1);
    while (vlen && ((eq+1)[vlen-1] == '\n' || (eq+1)[vlen-1] == '\r' || (eq+1)[vlen-1] == ' ')) vlen--;

    if (klen >= ksz || vlen >= vsz) return -1;
    memcpy(key, line, klen); key[klen] = '\0';
    memcpy(val, eq + 1, vlen); val[vlen] = '\0';
    return 0;
}

const char* risolvi_percorso_config(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--config=", 9) == 0) {
            const char* p = argv[i] + 9;
            if (*p) return p;
        }
    }
    return PERCORSO_FILE_CONFIG;
}

void carica_configurazione(const char* percorso,
                           int* num_operatori, int* num_utenti, int* num_sportelli,
                           int* durata_sim, int* ns_per_minuto, int* max_pause,
                           float* p_min, float* p_max, int* soglia_attesa, int* durata_giorno,
                           int* num_richieste)
{
    int   d_num_workers      = 6;
    int   d_num_users        = 20;
    int   d_num_worker_seats = 10;
    int   d_sim_duration     = 3;           
    int   d_n_nano_secs      = 30000000;    
    int   d_max_pause        = 2;           
    float d_p_serv_min       = 0.30f;
    float d_p_serv_max       = 0.70f;
    int   d_explode_threshold = 999999;
    int   d_day_duration     = 60;        
    int   d_n_requests       = 1;
    

    FILE* f = fopen(percorso ? percorso : PERCORSO_FILE_CONFIG, "r");
    if (f) {
        char line[256], k[128], v[128];
        while (fgets(line, sizeof(line), f)) {
            if (line[0] == '#' || line[0] == ';' || line[0] == '\n') continue;
            if (parse_line_kv(line, k, sizeof(k), v, sizeof(v)) != 0) continue;

            if (!strcasecmp(k, "NOF_WORKERS"))
                d_num_workers = atoi(v);
            else if (!strcasecmp(k, "NOF_USERS"))
                d_num_users = atoi(v);
            else if (!strcasecmp(k, "NOF_WORKER_SEATS"))
                d_num_worker_seats = atoi(v);
            else if (!strcasecmp(k, "SIM_DURATION"))
                d_sim_duration = atoi(v);
            else if (!strcasecmp(k, "N_NANO_SECS"))
                d_n_nano_secs = atoi(v);
            else if (!strcasecmp(k, "NOF_PAUSE"))
                d_max_pause = atoi(v);
            else if (!strcasecmp(k, "P_SERV_MIN"))
                d_p_serv_min = (float)atof(v);
            else if (!strcasecmp(k, "P_SERV_MAX"))
                d_p_serv_max = (float)atof(v);
            else if (!strcasecmp(k, "EXPLODE_THRESHOLD"))
                d_explode_threshold = atoi(v);
            else if (!strcasecmp(k, "DAY_DURATION"))
                d_day_duration = atoi(v);
            else if (!strcasecmp(k, "N_REQUESTS"))
                d_n_requests = atoi(v);
        }
        fclose(f);
    } else {
        fprintf(stderr, "[carica_configurazione] Impossibile aprire %s, uso i valori predefiniti.\n",
                percorso ? percorso : PERCORSO_FILE_CONFIG);
    }

    if (num_operatori)    *num_operatori    = d_num_workers;
    if (num_utenti)       *num_utenti       = d_num_users;
    if (num_sportelli)    *num_sportelli    = d_num_worker_seats;
    if (durata_sim)       *durata_sim       = d_sim_duration;
    if (ns_per_minuto)    *ns_per_minuto    = d_n_nano_secs;
    if (max_pause)        *max_pause        = d_max_pause;
    if (p_min)            *p_min            = d_p_serv_min;
    if (p_max)            *p_max            = d_p_serv_max;
    if (soglia_attesa)    *soglia_attesa    = d_explode_threshold;
    if (durata_giorno)    *durata_giorno    = d_day_duration;
    if (num_richieste)    *num_richieste    = (d_n_requests < 1 ? 1 : d_n_requests);
    
}
