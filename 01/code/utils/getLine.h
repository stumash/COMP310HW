#ifndef GETLINE_H
#define GETLINE_H

#include <sys/types.h>
#include <stdio.h>

/**
 * Read line of text from input stream into buffer size n.
 * Line ends at EOF or buffer.
 *
 * buffer: stores chars being read
 * n: size of buffer
 * stream: source of input chars
 *
 * returns: length of first line read
 *
 * error codes:
 *      -1: if length of first line in stream >= n
 */
int getLine(char *buffer, int n, FILE *stream);

/**
 * returns: true if c == '\0' || c == '\n' || c == EOF
 */
static
int charIsEndOfString(char c);

#endif /* ifndef GETLINE_H */
