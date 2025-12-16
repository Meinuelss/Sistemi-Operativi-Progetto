##MAKE FILE

# Cartelle
SRC_DIR   = src
BUILD_DIR = build
BIN_DIR   = bin

# File di configurazione
CONFIG ?= config.conf

# Compilatore e flag
CC       = gcc
CFLAGS   = -O2 -Wall -Wextra -Werror -Wvla -D_GNU_SOURCE

# Sorgenti comuni
COMMON_SRCS = \
  memoria_condivisa.c \
  semafori.c \
  utilita.c \
  messaggi.c \
  coda.c \
  config.c \
  statistiche.c

# Oggetti comuni
COMMON_OBJS = $(COMMON_SRCS:%.c=$(BUILD_DIR)/%.o)

# Eseguibili
EXE_DISPENSER = $(BIN_DIR)/erogatore_ticket
EXE_OPER      = $(BIN_DIR)/operatore
EXE_UTENTE    = $(BIN_DIR)/utente
EXE_DIR       = $(BIN_DIR)/direttore
EXE_ADDUSR    = $(BIN_DIR)/aggiungi_utenti

# Sorgenti specifici
DISPENSER_SRCS = erogatore_ticket.c
OPER_SRCS      = operatore.c
UTENTE_SRCS    = utente.c
DIR_SRCS       = direttore.c
ADDUSR_SRCS    = aggiungi_utenti.c

# Oggetti specifici
DISPENSER_OBJS 	= $(DISPENSER_SRCS:%.c=$(BUILD_DIR)/%.o)
OPER_OBJS    	= $(OPER_SRCS:%.c=$(BUILD_DIR)/%.o)
UTENTE_OBJS  	= $(UTENTE_SRCS:%.c=$(BUILD_DIR)/%.o)
DIR_OBJS     	= $(DIR_SRCS:%.c=$(BUILD_DIR)/%.o)
ADDUSR_OBJS 	= $(ADDUSR_SRCS:%.c=$(BUILD_DIR)/%.o)

# Target di default
all: dirs $(EXE_DIR) $(EXE_DISPENSER) $(EXE_OPER) $(EXE_UTENTE) $(EXE_ADDUSR)

# Directory di build/bin
dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

# Regola generica di compilazione
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link eseguibili
$(EXE_DISPENSER): $(COMMON_OBJS) $(DISPENSER_OBJS)
	$(CC) $^ -o $@

$(EXE_OPER): $(COMMON_OBJS) $(OPER_OBJS)
	$(CC) $^ -o $@ -lm

$(EXE_UTENTE): $(COMMON_OBJS) $(UTENTE_OBJS)
	$(CC) $^ -o $@

$(EXE_DIR): $(COMMON_OBJS) $(DIR_OBJS)
	$(CC) $^ -o $@

$(EXE_ADDUSR): $(COMMON_OBJS) $(ADDUSR_OBJS)
	$(CC) $^ -o $@


# Esecuzione
run: all
	@cfg="$(CONFIG)"; \
	if [ ! -f "$$cfg" ]; then \
	  echo "ERRORE: File di configurazione '$$cfg' non trovato!"; \
	  exit 1; \
	fi; \
	echo ">> Avvio con configurazione: $$cfg"; \
	"$(EXE_DIR)" --config="$$cfg"

test-explode: all
	@echo ">> Test terminazione EXPLODE con config_explode.conf"
	@if [ ! -f "config_explode.conf" ]; then \
	  echo "ERRORE: config_explode.conf non trovato!"; \
	  exit 1; \
	fi; \
	"$(EXE_DIR)" --config=config_explode.conf 

# ---- Pulizia ----
clean: clean-ipc
	@echo ">> Pulizia oggetti..."
	@echo ">> Pulizia eseguibili..."
	@rm -f $(BUILD_DIR)/*
	@rm -f $(BIN_DIR)/*

clean-ipc:
	@echo ">> Rimozione risorse IPC..."
	@ipcs -m | grep $$(whoami) | awk '{print $$2}' | xargs -n1 ipcrm -m 2>/dev/null || true
	@ipcs -s | grep $$(whoami) | awk '{print $$2}' | xargs -n1 ipcrm -s 2>/dev/null || true
	@ipcs -q | grep $$(whoami) | awk '{print $$2}' | xargs -n1 ipcrm -q 2>/dev/null || true
	@echo ">> IPC pulito"

