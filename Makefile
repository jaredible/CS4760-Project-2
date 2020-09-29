CC	= gcc
CFLAGS	= -g
TARGET1	= master
TARGET2	= palin
OBJS1	= master.o
OBJS2	= palin.o

.SUFFIXES: .c .o

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS1)
	$(CC) -o $@ $(OBJS1)

$(TARGET2): $(OBJS2)
	$(CC) -o $@ $(OBJS2)

.c.o:
	$(CC) -c $(CFLAGS) $<

.PHONY: clean

clean:
	/bin/rm -f $(TARGET1) $(TARGET2) *.o *.out *.log
