#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "tree_to_asm.h"

#include "tree_ast.h"

static int AssembleNode (program_t *program, node_t *node, FILE *file);
static int AssembleKeyword (program_t *program, node_t *node, FILE *file);

int AssembleTreeToFile (program_t *program, const char *fileName)
{
    assert (program);

    DEBUG_LOG ("%s", "");

    FILE *file = fopen (fileName, "w");
    if (file == NULL)
    {
        ERROR_LOG ("Error opening file \"%s\" - %s", fileName, strerror (errno));
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_OPENING_FILE;
    }

    fprintf (file, "; This asm file was compiled from rap language - best language in the world!\n\n");

    fprintf (file, "CALL :main\n"
                   "HLT\n");

    TREE_DO_AND_RETURN (AssembleNode (program, program->ast.root, file));

    return TREE_OK;
}

int AssembleNode (program_t *program, node_t *node, FILE *file)
{
    assert (program);
    assert (node);
    assert (file);

    DEBUG_PTR (node);

    switch (node->type)
    {
        case TYPE_UKNOWN:
            ERROR_LOG ("%s", "Uknown type of node");

            return TREE_ERROR_INVALID_NODE;

        case TYPE_CONST_NUM:
            fprintf (file, "PUSH " VALUE_NUMBER_FSTRING "\n", node->value.number);

            break;
            
        case TYPE_KEYWORD:
            return AssembleKeyword (program, node, file);

        case TYPE_VARIABLE:
            fprintf (file, "PUSH %lu\n"
                           "POPR RAX\n"
                           "PUSHM [RAX]\n\n", node->value.idx);
            
            break;
        
        case TYPE_NAME:
            assert (0 && "Чувак, ты не должен это ассемблировать");

        default:
            assert (0 && "Add new type to AssembleNode");
            break;
    }

    return TREE_OK;
}

int AssembleKeyword (program_t *program, node_t *node, FILE *file)
{
    assert (program);
    assert (node);
    assert (file);
    assert (node->type == TYPE_KEYWORD);

    static size_t ifCounter = 0;

    const keyword_t *keyword = FindKeywordByIdx ((keywordIdxes_t) node->value.idx);

    if (keyword->numberOfArgs >= 1)
        TREE_DO_AND_RETURN (AssembleNode (program, node->left, file));

    if (keyword->numberOfArgs == 2)
        TREE_DO_AND_RETURN (AssembleNode (program, node->right, file));

    switch (node->value.idx)
    {
        case KEY_ADD:
            fprintf (file, "ADD\n\n");
            break;

        case KEY_SUB:
            fprintf (file, "SUB\n\n");
            break;

        case KEY_MUL:
            fprintf (file, "MUL\n\n");
            break;

        case KEY_DIV:
            fprintf (file, "DIV\n\n");
            break;

        case KEY_INPUT:
            fprintf (file, "IN\n");
            break;

        case KEY_PRINT:
            fprintf (file, "OUT\n");
            break;

        case KEY_IF:
            fprintf (file, "; if\n");

            TREE_DO_AND_RETURN (AssembleNode (program, node->left, file));

            fprintf (file, "PUSH 0\n"
                           "JE :endif_%lu\n", ifCounter);
            
            TREE_DO_AND_RETURN (AssembleNode (program, node->right, file));

            fprintf (file, ":endif_%lu\n\n", ifCounter);

            ifCounter++;

            break;

        case KEY_DECLARATE:
        case KEY_ASSIGN:
            TREE_DO_AND_RETURN (AssembleNode (program, node->right, file));

            if (node->left->type != TYPE_VARIABLE)
            {
                ERROR_LOG ("%s", "Left child of declarate/assign node should be variable");
                ERROR_LOG ("(but this is copy of pointer...) node[%p]", node); // FIXME:

                return TREE_ERROR_INVALID_NODE;
            }

            fprintf (file, "PUSH %lu\n"
                           "POPR RAX\n"
                           "POPM [RAX]\n", node->left->value.idx);     
            break;
        
        case KEY_CONNECT:
            if (node->left != NULL)
                TREE_DO_AND_RETURN (AssembleNode (program, node->left, file));
            
            if (node->right != NULL)
                TREE_DO_AND_RETURN (AssembleNode (program, node->right, file));
            break;

        case KEY_FUNC:
        {
            node_t *arguments = node->left;
            assert (arguments);

            node_t *functionName = arguments->left;
            assert (functionName);

            const name_t *functionNameStr = NamesTableFindByIdx (&program->namesTable, functionName->value.idx);

            fprintf (file, "\n:%.*s\n", (int) functionNameStr->len, functionNameStr->name);

            if (arguments->right != NULL)
                TREE_DO_AND_RETURN (AssembleNode (program, arguments->right, file));

            node_t *body = node->right;
            assert (body);

            TREE_DO_AND_RETURN (AssembleNode (program, body, file));

            break;
        }

        case KEY_MAIN:
        {
            node_t *functionName = node->left;
            assert (functionName);

            fprintf (file, "\n:main\n");

            node_t *body = node->right;
            assert (body);

            TREE_DO_AND_RETURN (AssembleNode (program, body, file));

            break;
        }

        case KEY_RETURN:
            assert (node->left);

            fprintf (file, "\n; Calctulating return value\n");

            TREE_DO_AND_RETURN (AssembleNode (program, node->left, file));

            fprintf (file, "POPR RAX\n"
                           "RET\n\n");
            break;

        case KEY_COMMA:
            assert (0 && "TODO:");

    
        default:
            assert (0 && "Add new keyword to AssembleKeyword()");
            break;
    }

    return TREE_OK;
}