#include <stdio.h>

#include "debug.h"

#include "tree.h"
#include "tree_ast.h"
#include "tree_load_prefix.h"
#include "tree_to_asm.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        ERROR_PRINT ("Launch program like this: %s tree_file.ast", argv[0]);

        return 1;
    }

    program_t program = {};

    TREE_DO_AND_RETURN (ProgramCtor (&program));

    TREE_DO_AND_CLEAR (TreeLoadPrefixFromFile (&program, &program.ast, argv[1]),
                       ProgramDtor (&program));

    TREE_DO_AND_RETURN (AssembleTreeToFile (&program, kDefaultAsmFile));

    ProgramDtor (&program);

    DEBUG_PRINT ("\n%s returned 0!\n", argv[0]);

    return 0;
}