#ifndef GETCMD
#define GETCMD

#include <stdlib.h>
#include <string.h>

#include <utils/getLine.h>
#include <utils/minTokenLen.h>
#include <utils/maxTokenLen.h>

#include <utils/constants.h>

/**
 * Print "sh> ". Read line of input on stdin.  Parse input
 * into tokens.
 *
 * n_tokens: the number of tokens parsed
 * len: the length of the token array
 *
 * returns: array of string tokens
 *          n_tokens
 *          len
 */
char **getCmdTokens(int *n_tokens, int *len);

/**
 * Free all token strings and the array that contains
 * pointers to them.
 *
 * tokens: the array of string tokens
 * n_tokens: the number of tokens in the tokens array
 * len: the length of the token array
 */
void freeCmdTokens(char **tokens, int n_tokens, int len);

#endif /* ifndef GETCMD */
