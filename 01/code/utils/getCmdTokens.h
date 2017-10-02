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
 *          number of tokens as int n_tok
 *          length of tokens as int tok_len
 */
void
getCmdTokens(char **args, int *n_tok, int *tok_len);

#endif /* ifndef GETCMD */
