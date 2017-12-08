CC = gcc

INCL_DIRS = -I$(dir $(realpath $(MAKEFILE_LIST))) # absolute dirname of makefile
SETT = -Wextra -Wpedantic -Werror -Wshadow
PTHREAD = -pthread
DEBUG =
CFLAGS = $(SETT) $(INCL_DIRS) $(PTHREAD) $(DEBUG)

LDFLAGS = -lrt -lm

A2_FALL2017_DEPS = a2_fall2017.c utils/getCmdTokens.o utils/getLine.o utils/minTokenLen.o utils/maxTokenLen.o
a2_fall2017: $(A2_FALL2017_DEPS)
	$(CC) $(A2_FALL2017_DEPS) -o $(@) $(CFLAGS) $(LDFLAGS)

GETCMDTOKENS_DEPS = utils/getCmdTokens.c
utils/getCmdTokens.o: $(GETCMDTOKENS_DEPS) utils/getCmdTokens.h
	$(CC) -c $(GETCMDTOKENS_DEPS) -o $(@) $(CFLAGS)

GETLINE_DEPS = utils/getLine.c
utils/getLine.o: $(GETLINE_DEPS) utils/getLine.h
	$(CC) -c $(GETLINE_DEPS) -o $(@) $(CFLAGS)

MINTOKENLEN_DEPS = utils/minTokenLen.c
utils/minTokenLen.o: $(MINTOKENLEN_DEPS) utils/minTokenLen.h
	$(CC) -c $(MINTOKENLEN_DEPS) -o $(@) $(CFLAGS)

MAXTOKENLEN_DEPS = utils/maxTokenLen.c
utils/maxTokenLen.o: $(MAXTOKENLEN_DEPS) utils/maxTokenLen.h
	$(CC) -c $(MAXTOKENLEN_DEPS) -o $(@) $(CFLAGS)

CONST_DEPS = utils/constants.c
utils/constants.o: $(CONST_DEPS) utils/constants.h
	$(CC) -c $(CONST_DEPS) -o $(@) $(CFLAGS)
