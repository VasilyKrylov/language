#ifndef K_TOKENIZATOR_H
#define K_TOKENIZATOR_H

#include "tree_ast.h"

int GetTokens (const char *fileName, program_t *program);

int TokensArrayCtor (tokensArray_t *tokens);
void TokensArrayDtor (tokensArray_t *tokens);

int VariablesTableCtor (variablesTable_t *namesTable);
void VariablesTableDtor (variablesTable_t *namesTable);

#endif // K_TOKENIZATOR