#ifndef MYGETLINE_H
#define MYGETLINE_H value

#include <sys/types.h>
#include <stdio.h>

/**
 * Read a line of text from an input stream into a buffer.
 * Read no more than n-1 bytes (leave room for '\0').
 * Stop reading at EOF or newline, deleting them from buffer.
 *
 * buffer: the buffer to read the line of text into
 * n: the maximum number of chars to read into the buffer
 * stream: the source of the text to be read
 *
 * returns: the length of the line that was read
 */
size_t mygetline(char *buffer, size_t n, FILE *stream);

#endif /* ifndef MYGETLINE_H */
