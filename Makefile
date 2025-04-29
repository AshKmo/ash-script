MODULE_INCLUDE_FLAGS := $(addprefix -I,$(shell echo modules/*))

clmrgn :
	gcc -o bin/clmrgn $(MODULE_INCLUDE_FLAGS) modules/*/*.c main.c

debug :
	gcc -g -o bin/clmrgn $(MODULE_INCLUDE_FLAGS) modules/*/*.c main.c

clean :
	rm bin/*
