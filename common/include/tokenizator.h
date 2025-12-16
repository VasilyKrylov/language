#ifndef K_TOKENIZATOR_H
#define K_TOKENIZATOR_H

#include "tree_ast.h"

int GetTokens (const char *fileName, program_t *program);

int TokensArrayCtor (tokensArray_t *tokens);
void TokensArrayDtor (tokensArray_t *tokens);

int VariablesTableCtor (variablesTable_t *namesTable);
void VariablesTableDtor (variablesTable_t *namesTable);

void DumpTokens    (program_t *program);
void DumpVariables (variablesTable_t *variables);
int PrintToken (FILE *file, program_t *program, token_t *token);

#endif // K_TOKENIZATOR