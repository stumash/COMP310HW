CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -Wshadow

MAIN_DEPS = main.c mygetline/mygetline.o
main: $(MAIN_DEPS)
	$(CC) $(MAIN_DEPS) -o $(@) $(CFLAGS)

MYGETLINE_DEPS = mygetline/mygetline.c
mygetline/mygetline.o: $(MYGETLINE_DEPS)
	$(CC) -c $(MYGETLINE_DEPS) -o $(@) $(CFLAGS)
