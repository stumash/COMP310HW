#ifndef MAXTOKENLEN_H
#define MAXTOKENLEN_H

#include <sys/types.h>

/**
 * Determine length of largest space-delimited token in string
 *
 * s: parse tokens from here
 * slen: length of string
 *
 * returns: length of largest token
 */
ssize_t maxTokenLen(char *s, int slen);

#endif /* ifndef MAXTOKENLEN_H */
