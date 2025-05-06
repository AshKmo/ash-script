MODULE_INCLUDE_FLAGS := $(addprefix -I,$(shell echo modules/*))

clmrgn :
	$(CC) -o bin/ash-script -lm $(MODULE_INCLUDE_FLAGS) modules/*/*.c main.c

debug :
	$(CC) -g -o bin/ash-script -lm $(MODULE_INCLUDE_FLAGS) modules/*/*.c main.c

clean :
	rm -f bin/*
