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
ssize_t getLine(char *buffer, size_t n, FILE *stream);

#endif /* ifndef GETLINE_H */
