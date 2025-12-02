#include <stdio.h>

#include "tree.h"
#include "tree_ast.h"

int main()
{
    program_t program = {};

    TREE_DO_AND_RETURN (ProgramCtor (&program, 10));

    ProgramDtor (&program);

    return 0;
}