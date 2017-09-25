CC = gcc

$(info $(DEBUG))

INCL_DIRS = -I$(dir $(realpath $(MAKEFILE_LIST))) # absolute dirname of makefile
C11 = -std=c11
SETT = -Wextra -Wpedantic -Werror -Wshadow
CFLAGS = $(C11) $(SETT) $(INCL_DIRS) $(DEBUG)

MAIN_DEPS = main.c utils/getLine.o utils/minTokenLen.o utils/maxTokenLen.o
main.o: $(MAIN_DEPS)
	$(CC) $(MAIN_DEPS) -o $(@) $(CFLAGS)

GETLINE_DEPS = utils/getLine.c
utils/getLine.o: $(GETLINE_DEPS)
	$(CC) -c $(GETLINE_DEPS) -o $(@) $(CFLAGS)

MINTOKENLEN_DEPS = utils/minTokenLen.c
utils/minTokenLen.o: $(MINTOKENLEN_DEPS)
	$(CC) -c $(MINTOKENLEN_DEPS) -o $(@) $(CFLAGS)

MAXTOKENLEN_DEPS = utils/maxTokenLen.c
utils/maxTokenLen.o: $(MAXTOKENLEN_DEPS)
	$(CC) -c $(MAXTOKENLEN_DEPS) -o $(@) $(CFLAGS)
