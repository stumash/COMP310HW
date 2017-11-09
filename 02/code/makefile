CC = gcc

INCL_DIRS = -I$(dir $(realpath $(MAKEFILE_LIST))) # absolute dirname of makefile
SETT = -Wextra -Wpedantic -Werror -Wshadow
CFLAGS = $(SETT) $(INCL_DIRS)

LDFLAGS = -lrt

A2_FALL2017_DEPS = a2_fall2017.c
a2_fall2017: $(A2_FALL2017_DEPS)
	$(CC) $(A2_FALL2017_DEPS) -o $(@) $(CFLAGS) $(LDFLAGS)
