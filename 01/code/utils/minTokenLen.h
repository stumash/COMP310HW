#ifndef MIN_TOKEN_LEN
#define MIN_TOKEN_LEN

#include <sys/types.h>

/**
 * Determine length of smallest space-delimited token in string
 *
 * s: parse tokens from here
 * slen: length of string
 *
 * returns: length of smallest token
 */
int minTokenLen(char *s, int slen);

#endif // ifndef MIN_TOKEN_LEN
