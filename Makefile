CFLAGS = -I modules/Stack/ -I modules/String/

ash-script : main.o Stack.o String.o
	$(CC) $(CFLAGS) -o bin/ash-script -lm build/*

debug : CFLAGS += -g
debug : ash-script

wasm : main.o Stack.o String.o
	$(CC) $(CFLAGS) -o wasm/ash-script.wasm -lm build/*

main.o : main.c
	$(CC) $(CFLAGS) -c -o build/main.o main.c

Stack.o : modules/Stack/Stack.c
	$(CC) $(CFLAGS) -c -o build/Stack.o modules/Stack/Stack.c

String.o : modules/String/String.c
	$(CC) $(CFLAGS) -c -o build/String.o modules/String/String.c

clean :
	rm -f bin/* build/* wasm/ash-script.wasm
