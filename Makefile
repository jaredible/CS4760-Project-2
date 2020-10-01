CC		= gcc
CFLAGS		= -Wall -g

HEADER		= shared.h

MASTER_SRC	= master.c
MASTER_OBJ	= $(MASTER_SRC:.c=.o)
MASTER		= master

OBJ		= shared.o

PALIN_SRC	= palin.c
PALIN_OBJ	= $(PALIN_SRC:.c=.o)
PALIN		= palin

OUTPUT		= $(MASTER) $(PALIN)
all: $(OUTPUT)

$(MASTER): $(MASTER_OBJ) $(OBJ)
	$(CC) $(CFLAGS) $(MASTER_OBJ) $(OBJ) -o $(MASTER)

$(PALIN): $(PALIN_OBJ) $(OBJ)
	$(CC) $(CFLAGS) $(PALIN_OBJ) $(OBJ) -o $(PALIN)

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $*.c -o $*.o

.PHONY: clean
clean:
	/bin/rm -f $(OUTPUT) *.o *.out *.log
