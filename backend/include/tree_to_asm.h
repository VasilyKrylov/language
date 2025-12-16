#ifndef K_TREE_TO_ASM_H
#define K_TREE_TO_ASM_H

#include "tree_ast.h"

const char * const kDefaultAsmFile = "../processor/asm/my_asm/lang_auto_compiled.my_asm";

int AssembleTreeToFile (program_t *program, const char *fileName);

#endif // K_TREE_TO_ASM