#ifndef K_TREE_LOAD_INFIX
#define K_TREE_LOAD_INFIX

#include "tree.h"
#include "tree_ast.h"

int TreeLoadInfixFromFile (program_t *program, tree_t *tree,
                           const char *fileName, char **buffer, size_t *bufferLen);

#endif // K_TREE_LOAD_INFIX