#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tree.h"
#include "tree_ast.h"
#include "tokenizator.h"
#include "tree_load_infix.h"

void DebugTokens (program_t *program);
void DebugVariables (program_t *program);

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

    // DebugTokens    (&program);
    // DebugVariables (&program);

    TREE_DO_AND_CLEAR (TreeLoadInfixFromFile (&program),
                       ProgramDtor (&program));

    DEBUG_STR ("PIZDA");

    TREE_DO_AND_CLEAR (TreeAstSaveToFile (&program, "tree.txt"),
                       ProgramDtor (&program));

    ProgramDtor (&program);

    return 0;
}

void DebugTokens (program_t *program)
{
    assert (program);

    for (size_t i = 0; i < program->tokens.size; i++)
    {
        token_t *token = &program->tokens.data[i];
        DEBUG_LOG ("token[%lu]:", i);
        DEBUG_LOG ("\t type = %s", GetTypeName (token->type));

        if (token->type == TYPE_CONST_NUM)
            DEBUG_LOG ("\t value.number = " VALUE_NUMBER_FSTRING, token->value.number);
        else
            DEBUG_LOG ("\t value.idx = %lu", token->value.idx);

        DEBUG_LOG ("\t line = %lu", token->line);
        DEBUG_LOG ("\t position = %lu", token->line);
    }
}

void DebugVariables (program_t *program)
{
    assert (program);

    for (size_t i = 0; i < program->variables.size; i++)
    {
        variable_t *var = &program->variables.data[i];

        DEBUG_LOG ("variables[%lu]:", i);
        DEBUG_LOG ("\t idx = %lu", var->idx);
        DEBUG_LOG ("\t name = \"%s\"", var->name);
        DEBUG_LOG ("\t len = %lu", var->len);
    }
}