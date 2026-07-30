#include "messaggi.h"
#include "utilita.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static key_t get_msg_key(void) {
    key_t k = ftok(MSG_FTOK_PERCORSO, MSG_FTOK_PROGETTO);
    if (k == (key_t)-1) TEST_ERROR(-1, "ftok messages");
    
    return k;
}

int crea_coda_messaggi(void) {
    key_t k = get_msg_key();
    
    int msgid = msgget(k, IPC_CREAT | 0666);
    if (msgid == -1) {
        TEST_ERROR(-1, "msgget create");
    }
    return msgid;
}

int rimuovi_coda_messaggi(int msgid) {
    if (msgid < 0) return 0;
    
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        if (errno != EIDRM && errno != EINVAL) {
            perror("msgctl IPC_RMID");
            return -1;
        }
    }
    return 0;
}

int invia_risposta(int msgid, pid_t pid_destinatario, int esito, int tipo_servizio, int giorno) {
    MessaggioRisposta r;
    r.mtype = (long)pid_destinatario;
    r.esito = esito;
    r.tipo_servizio = tipo_servizio;
    r.giorno = giorno;

    if (msgsnd(msgid, &r, sizeof(r) - sizeof(long), 0) == -1) {
        if (errno != EINTR && errno != EIDRM && errno != EINVAL) {
            perror("invia_risposta: msgsnd");
        }
        return -1;
    }
    return 0;
}

int ricevi_risposta_per_pid(int msgid, MessaggioRisposta* out, int flags) {
    long mytype = (long)getpid();
    
    ssize_t n = msgrcv(msgid, out, sizeof(*out) - sizeof(long), mytype, flags);
    
    if (n == -1) {
        // Non stampiamo errore se interrotto o coda rimossa
        if (errno != EINTR && errno != EIDRM && errno != EINVAL && errno != ENOMSG) {
            perror("ricevi_risposta: msgrcv");
        }
        return -1;
    }
    return 0;
}