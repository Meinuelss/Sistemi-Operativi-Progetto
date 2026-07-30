#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>

static void safe_copy_trim(char* dest, const char* src, size_t n, size_t dest_sz) {
    if (n >= dest_sz) n = dest_sz - 1;
    
    strncpy(dest, src, n);
    dest[n] = '\0'; 

    for (int i = n - 1; i >= 0; i--) {
        if (isspace((unsigned char)dest[i])) dest[i] = '\0';
        else break;
    }
}

static int parse_line_kv(const char* line, char* key, size_t ksz, char* val, size_t vsz) {
    const char* eq = strchr(line, '=');
    if (!eq) return -1;
    
    size_t klen = (size_t)(eq - line);
    while (klen > 0 && isspace((unsigned char)line[klen-1])) klen--;

    const char* vstart = eq + 1;
    while (*vstart && isspace((unsigned char)*vstart) && *vstart != '\n') vstart++;
    
    safe_copy_trim(key, line, klen, ksz);
    safe_copy_trim(val, vstart, strlen(vstart), vsz);

    return 0;
}

const char* risolvi_percorso_config(int argc, char** argv) {
    const char* env_path = getenv("POSTE_CONFIG");
    if (env_path && strlen(env_path) > 0) return env_path;

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
    
    const char* path = percorso ? percorso : PERCORSO_FILE_CONFIG;
    FILE* f = fopen(path, "r");
    
    if (f) {
        printf("[Config] Caricamento da: %s\n", path);
        char line[256], k[128], v[128];
        
        while (fgets(line, sizeof(line), f)) {
            char* p = line;
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == '#' || *p == ';' || *p == '\0') continue;

            if (parse_line_kv(line, k, sizeof(k), v, sizeof(v)) != 0) continue;

            if (!strcasecmp(k, "NOF_WORKERS"))        d_num_workers = atoi(v);
            else if (!strcasecmp(k, "NOF_USERS"))     d_num_users = atoi(v);
            else if (!strcasecmp(k, "NOF_WORKER_SEATS")) d_num_worker_seats = atoi(v);
            else if (!strcasecmp(k, "SIM_DURATION"))  d_sim_duration = atoi(v);
            else if (!strcasecmp(k, "N_NANO_SECS"))   d_n_nano_secs = atoi(v);
            else if (!strcasecmp(k, "NOF_PAUSE"))     d_max_pause = atoi(v);
            else if (!strcasecmp(k, "P_SERV_MIN"))    d_p_serv_min = (float)atof(v);
            else if (!strcasecmp(k, "P_SERV_MAX"))    d_p_serv_max = (float)atof(v);
            else if (!strcasecmp(k, "EXPLODE_THRESHOLD")) d_explode_threshold = atoi(v);
            else if (!strcasecmp(k, "DAY_DURATION"))  d_day_duration = atoi(v);
            else if (!strcasecmp(k, "N_REQUESTS"))    d_n_requests = atoi(v);
        }
        fclose(f);
    } else {
        fprintf(stderr, "[Config] Impossibile aprire %s. Uso valori predefiniti.\n", path);
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