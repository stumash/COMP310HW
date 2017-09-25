CC = gcc

INCL_DIRS = -I$(dir $(realpath $(MAKEFILE_LIST)))
C11 = -std=c11
SETT = -Wextra -Wpedantic -Werror -Wshadow
CFLAGS = $(C11) $(SETT) $(INCL_DIRS)

MAIN_DEPS = main.c utils/getLine.o
main.o: $(MAIN_DEPS)
	$(CC) $(MAIN_DEPS) -o $(@) $(CFLAGS)

GETLINE_DEPS = utils/getLine.c
utils/getLine.o: $(GETLINE_DEPS)
	$(CC) -c $(GETLINE_DEPS) -o $(@) $(CFLAGS)
