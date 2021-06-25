OBJS=main.o adc_if.o util.o

CFLAGS=
LIBS=-lpthread -lm
CC=g++

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)
main:	$(OBJS)
	$(CC) $(LIBS) $(OBJS) -o main $(CFLAGS)
all:	main
clean:
	rm -fv main *.o
