# Makefile for t-omdbapi.c
#
CC=gcc
HDRS_DIR =./
CFLAGS=-I$(HDRS_DIR)

#LIBS=-lm
LIBS=
BIN =./

SRCS = \
    src/t-omdbapi.c

SRCS_OBJ = \
    src/t-omdbapi.o

%.o: %.c 
    $(CC) -c -o $@ $< $(CFLAGS)

t-omdbapi: $(SRCS_OBJ)
    gcc $(CFLAGS) $(LIBS) $(SRCS) -o $(BIN)/$@

all: $(SRCS_OBJ) t-omdbapi
    
.PHONY: clean

clean:
    rm -f $(SRCS_OBJ) t-omdbapi
    
