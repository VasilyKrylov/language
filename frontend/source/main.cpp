#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tree.h"
#include "tree_ast.h"
#include "tokenizator.h"
#include "tree_load_infix.h"

#define MAIN_DO_AND_CLEAR(action, clearAction)                          \
        do                                                              \
        {                                                               \
            int statusMacro = action;                                   \
            DEBUG_VAR("%d", statusMacro);                               \
                                                                        \
            if (statusMacro != TREE_OK)                                 \
            {                                                           \
                clearAction;                                            \
                                                                        \
                DEBUG_VAR("%d", statusMacro);                           \
                ERROR_PRINT ("%s", "Error occured in \"" #action "\""); \
                return 1;                                               \
            }                                                           \
        } while (0)

#define MAIN_DO_AND_RETURN(action)                                      \
        do                                                              \
        {                                                               \
            int statusMacro = action;                                   \
            DEBUG_VAR("%d", statusMacro);                               \
                                                                        \
            if (statusMacro != TREE_OK)                                 \
            {                                                           \
                ERROR_PRINT ("%s", "Error occured in \"" #action "\""); \
                return 1;                                               \
            }                                                           \
        } while (0)


int main(int argc, char **argv)
{
    if (argc != 2)
    {
        ERROR_PRINT ("Launch program like this: %s source_file.rap", argv[0]);

        return 1;
    }

    program_t program = {};

    MAIN_DO_AND_RETURN (ProgramCtor (&program));

    
    MAIN_DO_AND_CLEAR (GetTokens (argv[1], &program),
                       ProgramDtor (&program));

    DumpTokens     (&program);
    NamesTableDump (&program.namesTable);

    MAIN_DO_AND_CLEAR (TreeLoadInfixFromTokens (&program),
                       ProgramDtor (&program));

    MAIN_DO_AND_CLEAR (TreeAstSaveToFile (&program, ktreeSaveFileName),
                       ProgramDtor (&program));

    ProgramDtor (&program);

    return 0;
}