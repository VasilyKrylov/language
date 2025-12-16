#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tree.h"
#include "tree_ast.h"
#include "tokenizator.h"
#include "tree_load_infix.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        ERROR_PRINT ("Launch program like this: %s source_file.rap", argv[0]);

        return 1;
    }

    program_t program = {};

    TREE_DO_AND_RETURN (ProgramCtor (&program));
    
    TREE_DO_AND_CLEAR (GetTokens (argv[1], &program),
                       ProgramDtor (&program));

    DumpTokens    (&program);
    DumpVariables (&program.variables);

    TREE_DO_AND_CLEAR (TreeLoadInfixFromFile (&program),
                       ProgramDtor (&program));

    TREE_DO_AND_CLEAR (TreeAstSaveToFile (&program, ktreeSaveFileName),
                       ProgramDtor (&program));

    ProgramDtor (&program);

    return 0;
}




