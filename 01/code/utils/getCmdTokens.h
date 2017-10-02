#ifndef GETCMD
#define GETCMD

#include <utils/getLine.h>
#include <utils/minTokenLen.h>
#include <utils/maxTokenLen.h>

#include <utils/constants.h>

/**
 * Print "sh> ". Read line of input on stdin.  Parse input
 * into tokens.
 *
 * returns: tokens as char **args
 *          length of tokens as int tok_len
 */
void
getCmdTokens(char **args, int *tok_len);

#endif /* ifndef GETCMD */
