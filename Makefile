MODULE_INCLUDE_FLAGS := $(addprefix -I,$(shell echo modules/*))

clmrgn :
	gcc -o bin/ash-script -lm $(MODULE_INCLUDE_FLAGS) modules/*/*.c main.c

debug :
	gcc -g -o bin/ash-script -lm $(MODULE_INCLUDE_FLAGS) modules/*/*.c main.c

clean :
	rm bin/*
