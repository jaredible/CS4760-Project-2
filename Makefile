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

strings.in:
	echo "a" > strings.in
	echo "ab" >> strings.in
	echo "a1A" >> strings.in
	echo "" >> strings.in
	echo "aB0Ba" >> strings.in

test: all strings.in
	./master -n 10 -s 4 -t 4 strings.in
	cat palin.out; echo; cat nopalin.out; echo; cat output.log
	rm palin.out nopalin.out output.log
	echo
	./master -n `wc -l <strings.in` -s 3 -t 2 strings.in
	cat palin.out; echo; cat nopalin.out; echo; cat output.log
