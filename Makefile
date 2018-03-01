CC=gcc
CFLAGS=-I.
DEPS = 
OBJ = gbn.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

gbn: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)
	
clean: 
	rm -f gbn.o gbn
