.SUFFUXES : .h .c .o

OBJS = hash.o hashtable.o tailall.o

CC = gcc
CFLAGS = -Wall -g -c -DDEBUG
#CFLAGS = -Wall -g -c
CFLAGS_RELEASE = -Wall -g -c

LDFLAGS	= 
INC = -I../include

SRCS = $(OBJS:.o=.c)

ARS = 

TARGET = tailall

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(ARS) $(LDFLAGS)

.c.o :
	$(CC) $(INC) $(CFLAGS) $<

run :
	./$(TARGET) .

gdb :
	gdb ./$(TARGET)

clean : 
	rm -rf $(OBJS) $(TARGET) core 

