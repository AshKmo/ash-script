CFLAGS = -I modules/Stack/ -I modules/String/

ash-script : main.o Stack.o String.o
	$(CC) $(CFLAGS) -o build/ash-script -lm build/*.o

debug : CFLAGS += -g
debug : ash-script

main.o : main.c
	$(CC) $(CFLAGS) -c -o build/main.o main.c

Stack.o : modules/Stack/Stack.c
	$(CC) $(CFLAGS) -c -o build/Stack.o modules/Stack/Stack.c

String.o : modules/String/String.c
	$(CC) $(CFLAGS) -c -o build/String.o modules/String/String.c

clean :
	rm -f bin/* build/*
