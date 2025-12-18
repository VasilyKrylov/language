#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tree_load_prefix.h"

#include "tree.h"
#include "tree_ast.h"
#include "utils.h"

static int TreeLoadNode             (program_t *program, node_t **node,
                                     char **curPos);
static int TreeLoadNodeAndFill      (program_t *program, node_t **node,
                                     char **curPos);
static int TreeLoadChildNodes       (program_t *program, node_t **node,
                                     char **curPos);
static int TreeLoadDetectNodeType   (program_t *program, char **curPos, int *readBytes,
                                     type_t *type, value_t *value);

int TreeLoadPrefixFromFile (program_t *program, tree_t *tree,
                            const char *fileName)
{
    assert (program);
    assert (tree);
    assert (fileName);

    DEBUG_PRINT ("\n========== LOADING TREE FROM \"%s\" ==========\n", fileName);

    if (tree->root != NULL)
    {
        ERROR_LOG ("%s", "TREE_ERROR_LOAD_INTO_NOT_EMPTY");
        
        return TREE_ERROR_LOAD_INTO_NOT_EMPTY;
    }
    
    size_t bufferLen = 0;
    program->buffer = ReadFile (fileName, &bufferLen);
    if (program->buffer == NULL)
        return TREE_ERROR_COMMON |
               COMMON_ERROR_READING_FILE;

    char *curPos = program->buffer;

    DEBUG_STR (fileName);
    DEBUG_STR (curPos);
    
    int status = TreeLoadNode (program, &tree->root, &curPos);

    if (status != TREE_OK)
    {
        ERROR_LOG ("%s", "Error in TreeLoadNode()");

        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    }

    TREE_DUMP (program, tree, "%s", "After load");
    
    DEBUG_PRINT ("%s", "==========    END OF LOADING TREE    ==========\n\n");

    return TREE_OK;
}

int TreeLoadNode (program_t *program, node_t **node,
                  char **curPos)
{
    assert (program);
    assert (node);
    assert (curPos);
    assert (*curPos);

    tree_t *tree = &program->ast;

    *curPos = SkipSpaces (*curPos);

    if (**curPos == '(')
    {
        NODE_CTOR (tree, *node);

        int status = TreeLoadNodeAndFill (program, node, curPos);

        return status;
    }
    else if (strncmp (*curPos, "nil", sizeof("nil") - 1) == 0)
    {
        *curPos += sizeof ("nil") - 1;

        *node = NULL;

        return TREE_OK;
    }
    else 
    {
        ERROR_LOG ("%s", "Syntax error in tree dump file - uknown beginning of the node");
        ERROR_LOG ("curPos = \"%s\";", *curPos);

        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    }
}


// FIXME: big function
int TreeLoadNodeAndFill (program_t *program, node_t **node,
                         char **curPos)
{
    assert (program);
    assert (node);
    assert (curPos);
    assert (*curPos);

    DEBUG_PRINT ("%s", "\n===== CREATING NEW NODE =====\n");
    // DEBUG_STR (*curPos);

    (*curPos)++; // move after '('

    *curPos = SkipSpaces (*curPos);
    
    int readBytes = 0;
    type_t type = TYPE_UKNOWN;
    value_t value = {};

    TREE_DO_AND_RETURN (
        TreeLoadDetectNodeType (program, curPos, &readBytes, &type, &value)
    );

    NodeFill (*node, type, value, NULL, NULL);
    
    *curPos += readBytes;
    // *curPos += 1; // because it can be '\0', not space
    // DEBUG_STR (*curPos);
    *curPos = SkipSpaces (*curPos);
    
    // DEBUG_STR (data);
    NODE_DUMP (program, *node, "Created new %s node. curPos = \'%s\'", GetTypeName (type), *curPos);

    int status = TreeLoadChildNodes (program, node, curPos);
    if (status != TREE_OK)
        return status;

    *curPos = SkipSpaces (*curPos);
    
    if (**curPos != ')')
    {
        ERROR_LOG ("%s", "Syntax error in tree dump file - missing closing bracket ')'");
        ERROR_LOG ("curPos = \'%s\';", *curPos);

        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    }

    (*curPos)++;

    return TREE_OK;
}

int TreeLoadDetectNodeType  (program_t *program, char **curPos, int *readBytes,
                             type_t *type, value_t *value)
{
    assert (program);
    assert (curPos);
    assert (*curPos);
    assert (readBytes);
    assert (type);
    assert (value);

    if (**curPos == '"')
    {
        (*curPos)++;

        char *varNameEnd = strchr (*curPos, '"');
        DEBUG_PTR (*curPos);
        DEBUG_PTR (varNameEnd);

        if (varNameEnd == NULL)
        {
            ERROR_PRINT ("%s", "Where is no closing double quote in name of variable");
            ERROR_PRINT ("curPos = \"%s\"", *curPos);

            return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
        }

        *readBytes = int (varNameEnd - *curPos);
        DEBUG_VAR ("%d", *readBytes);

        size_t idx = 0;
        TREE_DO_AND_RETURN (
            NamesTableFindOrAdd (&program->namesTable, *curPos, (size_t) *readBytes, &idx)
        );

        (*readBytes)++;

        *type = TYPE_VARIABLE;
        *value = {.idx = idx};
    }
    else if (sscanf (*curPos, VALUE_NUMBER_FSTRING "%n", &value->number, readBytes) == 1)
    {
        *type = TYPE_CONST_NUM;
        
        DEBUG_LOG ("number " VALUE_NUMBER_FSTRING " detected", value->number);
    }
    else
    {
        sscanf (*curPos, "%*s%n", readBytes);
        
        TryToFindNode (*curPos, *readBytes, type, value);
    }

    if (*type == TYPE_UKNOWN)
    {
        ERROR_PRINT ("Wtf bro, I don't know such node: \n\"%s\"", *curPos);

        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    }

    DEBUG_LOG ("%s", "After detecting type:");
    // DEBUG_STR (*curPos);
    DEBUG_LOG ("readBytes = %d", *readBytes);
    DEBUG_LOG ("type = %d", *type);
    DEBUG_LOG ("value.idx    = %lu", value->idx);
    DEBUG_LOG ("value.number = " VALUE_NUMBER_FSTRING, value->number);

    return TREE_OK;
}

int TreeLoadChildNodes (program_t *program, node_t **node,
                        char **curPos)
{
    assert (program);
    assert (node);
    assert (curPos);
    assert (*curPos);

    *curPos = SkipSpaces (*curPos);

    int status = TreeLoadNode (program, &(*node)->left, curPos);
    if (status != TREE_OK)
        return status;

    if ((*node)->left != NULL)
    {
        NODE_DUMP (program, (*node)->left, "After creating left subtree. \n"
                                        "curPos = \'%s\'", *curPos);
    }

    *curPos = SkipSpaces (*curPos);
    
    status = TreeLoadNode (program, &(*node)->right, curPos);
    if (status != TREE_OK)
        return status;

    if ((*node)->right != NULL)
    {
        NODE_DUMP (program, (*node)->right, "After creating right subtree. \n"
                                         "curPos = \'%s\'", *curPos);
    }
    
    return TREE_OK;
}

