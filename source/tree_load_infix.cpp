#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "tree_load_infix.h"

#include "tree.h"
#include "tree_ast.h"
#include "utils.h"

// if we believe https://en.wikipedia.org/wiki/Parsing_expression_grammar,
// ? means optional

/*
Examples+ = {"log ( 3 + x * 2) -    (7+ 3)*x ^ 2"}

Gramma      ::= Expression '\0'

Assign      ::= Variable '=' PrimaryExp

Expression  ::= Term        {['+', '-']Term}*
Term        ::= Power       {['/', '*']Power}*
// FIXME: power
Power       ::= PrimaryExp  {'^'PrimaryExp}* 
PrimaryExp  ::= '('Expression')' | Number | Function | Variable

Number      ::= ['0'-'9']+{'.'['0'-'9']+}?
Function    ::= ["sin", "cos", ...]'('Expression')' | 
                ["log"]'('Expression','Expression')'
Variable    ::= ['a'-'z', 'A'-'Z', '_']'a'-'z', 'A'-'Z', '0'-'9', '_']*

*/

/*


Examples+ = {"log ( 3 + x * 2) -    (7+ 3)*x ^ 2"}

Gramma      ::= Operation+ '\0'

Spaces      ::= {' '}*

Operation   ::= Spaces {If | '{' Opeartion+ '}' | Assign} Spaces
If          ::= "if" Spaces '(' E ')' Operation 
Assign      ::= Variable Spaces '=' PrimaryExp ';'

Expression  ::= Term        {['+', '-'] Spaces Term}* 
Term        ::= Power       {['/', '*'] Spaces Power}*
// FIXME: power
Power       ::= PrimaryExp  {'^' Spaces PrimaryExp}* 
PrimaryExp  ::= Spaces {'('  Expression ')' | Number | Function | Variable} Spaces

Number      ::= ['0'-'9']+{'.'['0'-'9']+}?
Function    ::= ["sin", "cos", ...] Spaces '(' Expression ')' | 
                ["log"] Spaces '(' Expression ',' Expression')'
Variable    ::= ['a'-'z', 'A'-'Z', '_']['a'-'z', 'A'-'Z', '0'-'9', '_']*
/
*/

#define SYNTAX_ERROR                                                    \
        {                                                               \
            ERROR_LOG ("Syntax Error on position \"%s\"", *curPos);     \
                                                                        \
            return TREE_ERROR_SYNTAX_IN_SAVE_FILE;                      \
        }

static int GetGramma            (program_t *program, char **curPos,
                                 tree_t *resTree, node_t **node);
static int GetOperation         (program_t *program, char **curPos,
                                 tree_t *resTree, node_t **node);
static int GetIf                (program_t *program, char **curPos,
                                 tree_t *resTree, node_t **node);
static int GetAssign            (program_t *program, char **curPos,
                                 tree_t *resTree, node_t **node);
static int GetExpression        (program_t *program, char **curPos,
                                 tree_t *resTree, node_t **node);
static int GetTerm              (program_t *program, char **curPos,
                                 tree_t *resTree, node_t **node);
static int GetPower             (program_t *program, char **curPos,
                                 tree_t *resTree, node_t **node);
static int GetPrimaryExpression (program_t *program, char **curPos,
                                 tree_t *resTree, node_t **node);
static int GetVariable          (program_t *program, char **curPos,
                                 tree_t *resTree, node_t **node);

static int GetFunction          (program_t *program, char **curPos,
                                 tree_t *resTree, node_t **node);
static int GetVariableName      (char **curPos);
static int GetNumber            (program_t *program, char **curPos,
                                 tree_t *resTree, node_t **node);

int TreeLoadInfixFromFile (program_t *program, tree_t *tree,
                           const char *fileName, char **buffer, size_t *bufferLen)
{
    assert (program);
    assert (tree);
    assert (fileName);
    assert (buffer);
    assert (bufferLen);

    DEBUG_PRINT ("\n========== LOADING TREE FROM \"%s\" ==========\n", fileName);

    if (tree->root != NULL)
    {
        ERROR_LOG ("%s", "TREE_ERROR_LOAD_INTO_NOT_EMPTY");
        
        return TREE_ERROR_LOAD_INTO_NOT_EMPTY;
    }

    *buffer = ReadFile (fileName, bufferLen);
    if (buffer == NULL)
        return TREE_ERROR_COMMON |
               COMMON_ERROR_READING_FILE;

    char *curPos = *buffer;
    
    // int status = GetGramma (program, &tree->root, *buffer, &curPos);
    int status = GetGramma (program, &curPos, tree, &tree->root);

    if (status != TREE_OK)
    {
        ERROR_LOG ("%s", "Error in GetGramma()");

        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    }

    TREE_DUMP (program, tree, "%s", "After load");
    
    DEBUG_PRINT ("%s", "==========    END OF LOADING TREE    ==========\n\n");

    return TREE_OK;
}

#include "dsl_define.h"

int GetGramma (program_t *program, char **curPos, tree_t *resTree, node_t **node)
{
    assert (program);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    int status = GetOperation (program, curPos, resTree, node);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    NODE_DUMP (program, *node, "Created new node. curPos = \'%s\'", *curPos);
    
    *curPos = SkipSpaces (*curPos);

    while (**curPos != '\0')
    {
        node_t *nodeNext = NULL;
        status = GetOperation (program, curPos, resTree, &nodeNext);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        *node = CONNECT_ (*node, nodeNext);

        NODE_DUMP (program, *node, "Created new node (connection). curPos = \'%s\'", *curPos);
        
        *curPos = SkipSpaces (*curPos);
    }

    if (**curPos != '\0')
        SYNTAX_ERROR;

    return TREE_OK;
}

int GetOperation (program_t *program, char **curPos, tree_t *resTree, node_t **node)
{
    assert (program);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    *curPos = SkipSpaces (*curPos);

    DEBUG_STR (*curPos);

    if (strncmp (*curPos, "if", 2) == 0 &&
        ((*curPos)[2] == '(' || isspace ((*curPos)[2])))
    {
        DEBUG_STR ("IF!!!!!!!!!");

        int status = GetIf (program, curPos, resTree, node);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        return TREE_OK;
    }

    DEBUG_STR ("NOT IF(((((");

    if (**curPos == '{')
    {
        DEBUG_STR ("{{{{{{{{{}}}}}}}}}");

        (*curPos)++;

        int status = GetOperation (program, curPos, resTree, node);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        *curPos = SkipSpaces (*curPos); // maybe remove

        while (**curPos != '}')
        {
            node_t *nodeArg = NULL;
            status = GetOperation (program, curPos, resTree, &nodeArg);
            if (status != TREE_OK)
                SYNTAX_ERROR;

            *node = CONNECT_ (*node, nodeArg);

            *curPos = SkipSpaces (*curPos); // maybe remove
        }

        (*curPos)++;

        return TREE_OK;
    }

    int status = GetAssign (program, curPos, resTree, node);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    *curPos = SkipSpaces (*curPos);

    return TREE_OK;
}

int GetIf (program_t *program, char **curPos, tree_t *resTree, node_t **node)
{
    assert (program);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    *curPos = SkipSpaces (*curPos);

    if (strncmp ("if", *curPos, 2) != 0)
        SYNTAX_ERROR;

    (*curPos) += 2;

    *curPos = SkipSpaces (*curPos);

    if (**curPos != '(')
        SYNTAX_ERROR;
    
    (*curPos)++;

    node_t *nodeL = NULL;
    int status = GetExpression (program, curPos, resTree, &nodeL);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    if (**curPos != ')')
        SYNTAX_ERROR;

    (*curPos)++;
    *curPos = SkipSpaces (*curPos);

    node_t *nodeR = NULL;
    status = GetOperation (program, curPos, resTree, &nodeR);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    *node = IF_ (nodeL, nodeR);

    NODE_DUMP (program, *node, "Created new node (if). curPos = \'%s\'", *curPos);

    return TREE_OK;
}


int GetAssign (program_t *program, char **curPos, tree_t *resTree, node_t **node)
{
    assert (program);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    *curPos = SkipSpaces (*curPos);

    // if (strncmp ("MC ", *curPos, 3) != )

    node_t *nodeL = NULL;
    int status = GetVariable (program, curPos, resTree, &nodeL);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    *curPos = SkipSpaces (*curPos);

    if (**curPos != '=')
        SYNTAX_ERROR;

    (*curPos)++;

    node_t *nodeR = NULL;
    status = GetExpression (program, curPos, resTree, &nodeR);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    *curPos = SkipSpaces (*curPos);
    
    if (**curPos != ';')
        SYNTAX_ERROR;
    
    (*curPos)++;
    *curPos = SkipSpaces (*curPos);

    *node = ASSIGN_ (nodeL, nodeR);

    DEBUG_STR (*curPos);

    NODE_DUMP (program, *node, "Created new node (assign). curPos = \'%s\'", *curPos);

    return TREE_OK;
}

int GetExpression (program_t *program, char **curPos, tree_t *resTree, node_t **node)
{
    assert (program);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    DEBUG_STR (*curPos);

    int status = GetTerm (program, curPos, resTree, node);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    while (**curPos == '+' ||
           **curPos == '-')
    {
        char operation = **curPos;
        (*curPos)++;

        *curPos = SkipSpaces (*curPos);

        node_t *node2 = NULL;
        status = GetTerm (program, curPos, resTree, &node2);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        switch (operation)
        {
            case '+': *node = ADD_ (*node, node2); break;
            case '-': *node = SUB_ (*node, node2); break;

            default: SYNTAX_ERROR;
        }
    }

    NODE_DUMP (program, *node, "Created new node (expression). curPos = \'%s\'", *curPos);

    return TREE_OK;
}

int GetTerm (program_t *program, char **curPos, tree_t *resTree, node_t **node)
{
    assert (program);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    int status = GetPower (program, curPos, resTree, node);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    while (**curPos == '*' ||
           **curPos == '/')
    {
        char operation = **curPos;
        (*curPos)++;

        *curPos = SkipSpaces (*curPos);

        node_t *node2 = NULL;
        status = GetPower (program, curPos, resTree, &node2);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        switch (operation)
        {
            case '*': *node = MUL_ (*node, node2); break;
            case '/': *node = DIV_ (*node, node2); break;

            default: SYNTAX_ERROR;
        }
    }

    DEBUG_LOG ("node [%p]",  (*node));
    DEBUG_LOG ("\tnode->left [%p]",  (*node)->left);
    DEBUG_LOG ("\tnode->right [%p]", (*node)->right);

    NODE_DUMP (program, *node, "Created new node (term). curPos = \'%s\'", *curPos);

    return TREE_OK;
}

int GetPower (program_t *program, char **curPos, 
              tree_t *resTree, node_t **node)
{
    assert (program);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    int status = GetPrimaryExpression (program, curPos, resTree, node);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    while (**curPos == '^')
    {
        (*curPos)++;

        *curPos = SkipSpaces (*curPos);

        node_t *node2 = NULL;
        status = GetPrimaryExpression (program, curPos, resTree, &node2);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        *node = POW_ (*node, node2);
    }

    return TREE_OK;
}

int GetPrimaryExpression (program_t *program, char **curPos, 
                          tree_t *resTree, node_t **node)
{
    assert (program);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    DEBUG_STR (*curPos);

    *curPos = SkipSpaces (*curPos);

    if (**curPos == '(')
    {
        (*curPos)++;
        
        int status = GetExpression (program, curPos, resTree, node);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        if (**curPos != ')')
            SYNTAX_ERROR;

        (*curPos)++;

        *curPos = SkipSpaces (*curPos);

        return status;
    }

    int status = GetNumber (program, curPos, resTree, node);
    if (status == TREE_OK)
    {
        *curPos = SkipSpaces (*curPos);

        return status;
    }

    status = GetFunction (program, curPos, resTree, node);
    if (status == TREE_OK)
    {
        *curPos = SkipSpaces (*curPos);
        
        return status;
    }
    
    status = GetVariable (program, curPos, resTree, node);
    DEBUG_LOG ("GetVariable() status = %d", status);
    if (status == TREE_OK)
    {
        *curPos = SkipSpaces (*curPos);
        
        return status;
    }

    DEBUG_STR (*curPos);

    SYNTAX_ERROR;
}

int GetNumber (program_t *program, char **curPos, tree_t *resTree, node_t **node)
{
    assert (program);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    double val = NAN;
    int readBytes = 0;

    int res = sscanf (*curPos, "%lf%n", &val, &readBytes);
    DEBUG_VAR ("%d", res);
    if (res != 1)
        return TREE_ERROR_NODE_NOT_FOUND;

    *curPos += readBytes;
    DEBUG_VAR ("%s", *curPos);
    DEBUG_VAR ("%g", val);

    *node = NUM_ (val);

    NODE_DUMP (program, *node, "Created new node (number). curPos = \'%s\'", *curPos);

    return TREE_OK;
}

int GetFunction (program_t *program, char **curPos, tree_t *resTree, node_t **node)
{
    assert (program);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    DEBUG_STR (*curPos);

    const keyword_t *func = NULL;

    for (size_t i = 0; i < kNumberOfKeywords; i++)
    {
        if (strncmp (*curPos, keywords[i].name, keywords[i].nameLen) == 0 &&
            keywords[i].isFunction)
        {
            *node = NodeCtorAndFill (resTree, 
                                     TYPE_MATH_OPERATION, 
                                     {.idx = (size_t) keywords[i].idx}, 
                                     NULL, NULL);

            NODE_DUMP (program, *node, "Created new node (function). curPos = \'%s\'", *curPos);
                                     
            *curPos += keywords[i].nameLen;
            func = &keywords[i];

            break;
        }
    }

    if (func == NULL)
    {
        DEBUG_LOG ("%s", "No function found. Return");

        return TREE_ERROR_INVALID_NODE;
    }

    if (func->numberOfArgs < 1 ||
        func->numberOfArgs > 2)
    {
        ERROR_LOG ("%s", "Where are functions with only 1 or 2 args now...\n"
                         "Maybe you forgot to rewrite this part of code?");

        return TREE_ERROR_INVALID_NODE;
    }

    *curPos = SkipSpaces (*curPos);

    DEBUG_STR (*curPos);

    if (**curPos != '(')
        SYNTAX_ERROR;
    
    (*curPos)++;

    node_t *firstArg = {};
    int status = GetExpression (program, curPos, resTree, &firstArg);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    DEBUG_STR (*curPos);

    if (func->numberOfArgs == 1)
    {
        if (**curPos != ')')
            SYNTAX_ERROR;
        
        (*curPos)++;
        (*node)->right = firstArg;

        return TREE_OK;
    }
    
    if (**curPos != ',')
        SYNTAX_ERROR;
    
    (*curPos)++;

    DEBUG_LOG ("*curPos = \"%s\" (after ',') ", *curPos);

    node_t *secondArg = {};
    status = GetExpression (program, curPos, resTree, &secondArg);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    if (**curPos != ')')
        SYNTAX_ERROR;

    (*curPos)++;
    DEBUG_STR (*curPos);

    (*node)->left  = firstArg;
    (*node)->right = secondArg;

    return TREE_OK;
}

int GetVariable (program_t *program, char **curPos, tree_t *resTree, node_t **node)
{
    assert (program);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    DEBUG_VAR ("%s", *curPos);

    char *varName = *curPos;
    DEBUG_VAR ("%p", varName);

    int res = GetVariableName (curPos);
    DEBUG_VAR ("%p", *curPos);

    if (res != TREE_OK)
    {
        DEBUG_LOG ("'%s' not a variable", *curPos);

        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    }

    type_t type   = {};
    value_t value = {};
    
    size_t varNameLen = size_t (*curPos - varName);
    DEBUG_VAR ("%lu", varNameLen);

    int status = FindOrAddVariable (program, &varName, varNameLen, &type, &value);
    if (status != TREE_OK)
        return status;

    *node = NodeCtorAndFill (resTree, type, value, NULL, NULL);

    DEBUG_LOG ("(after adding variable) *curPos = \"%s\"", *curPos);

    NODE_DUMP (program, *node, "Created new node (number). curPos = \'%s\'", *curPos);

    return TREE_OK;
}

int GetVariableName (char **curPos)
{
    assert (curPos);

    DEBUG_VAR ("%s", *curPos);

    DEBUG_LOG ("**curPos(variable name) = '%c'", **curPos);

    if (!isalpha (**curPos) && **curPos != '_')
        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    
    (*curPos)++;

    DEBUG_VAR ("%p", *curPos);

    while (isalpha (**curPos) || isdigit (**curPos) || **curPos == '_')
    {
        DEBUG_LOG ("**curPos(variable name) = \"%c\"", **curPos);
        DEBUG_VAR ("%p", *curPos);
        
        (*curPos)++;    
    }
    
    return TREE_OK;
}

#include "dsl_undef.h"

#undef SYNTAX_ERROR