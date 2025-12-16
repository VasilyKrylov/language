#ifndef K_TREE_LOAD_PREFIX
#define K_TREE_LOAD_PREFIX

#include "tree.h"
#include "tree_ast.h"

int TreeLoadPrefixFromFile (program_t *program, tree_t *tree,
                            const char *fileName);

#endif // K_TREE_LOAD_PREFIX