#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "tree_load_infix.h"

#include "tree.h"
#include "tree_ast.h"
#include "tokenizator.h"
#include "utils.h"

// if we believe https://en.wikipedia.org/wiki/Parsing_expression_grammar,
// ? means optional

/*


Examples+ = {"log ( 3 + x * 2) -    (7+ 3)*x ^ 2"}

Gramma      ::= Operation+ '\0'

Operation   ::=  {If | '{' Opeartion+ '}' | Declarate | Assign} 
If          ::= "if"  '(' E ')' Operation 
Declarate   ::= Variable  ':=' PrimaryExp ';'
Assign      ::= Variable   '=' PrimaryExp ';'

Expression  ::= Term        {['+', '-']  Term}* 
Term        ::= Power       {['/', '*']  Power}*
// FIXME: power
Power       ::= PrimaryExp  {'^'  PrimaryExp}* 
PrimaryExp  ::=  {'('  Expression ')' | Number | Function | Variable} 

Number      ::= ['0'-'9']+{'.'['0'-'9']+}?
Function    ::= ["sin", "cos", ...]  '(' Expression ')' | 
                ["log"]  '(' Expression ',' Expression')'
Variable    ::= ['a'-'z', 'A'-'Z', '_']['a'-'z', 'A'-'Z', '0'-'9', '_']*
/
*/

#define SYNTAX_ERROR                                                    \
        {                                                               \
            ERROR_LOG ("Syntax Error on line: %lu position: %lu",       \
                       tokens->data[*curToken].line,                    \
                       tokens->data[*curToken].position);               \
                                                                        \
            return TREE_ERROR_SYNTAX_IN_SAVE_FILE;                      \
        }

static int GetGramma            (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetOperation         (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetIf                (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetAssign            (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetExpression        (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetTerm              (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetPower             (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetPrimaryExpression (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetVariable          (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);

static int GetFunction          (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
// static int GetVariableName      (char **curPos);
static int GetNumber            (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);

int TreeLoadInfixFromFile (program_t *program)
{
    assert (program);

    DEBUG_PRINT ("%s", "\n========== LOADING TREE ==========\n");

    if (program->ast.root != NULL)
    {
        ERROR_LOG ("%s", "TREE_ERROR_LOAD_INTO_NOT_EMPTY");
        
        return TREE_ERROR_LOAD_INTO_NOT_EMPTY;
    }
    
    size_t curToken = 0;
    int status = GetGramma (program, &program->tokens, &curToken, &program->ast.root);

    if (status != TREE_OK)
    {
        ERROR_LOG ("%s", "Error in GetGramma()");

        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    }

    TREE_DUMP (program, &program->ast, "%s", "After load");
    
    DEBUG_PRINT ("%s", "==========    END OF LOADING TREE    ==========\n\n");

    return TREE_OK;
}

#include "dsl_define.h"

int GetGramma (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (node);

    int status = GetOperation (program, tokens, curToken, node);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    *node = CONNECT_ (*node, NULL);
    NODE_DUMP (program, *node, "Created new node (connection). curToken = %lu", *curToken);

    NODE_DUMP (program, *node, "Created new node. curToken = %lu", *curToken);

    while (*curToken < tokens->size)
    {
        node_t *nodeNext = NULL;

        status = GetOperation (program, tokens, curToken, &nodeNext);

        if (status != TREE_OK)
            SYNTAX_ERROR;

        // (*node)->right = CONNECT_ (nodeNext, NULL);
        // *node = (*node)->right;
        
        *node = CONNECT_ (*node, nodeNext);
        NODE_DUMP (program, (*node), "Created new node (connection). curToken = %lu", *curToken);
        // NODE_DUMP (program, (*node), "%s", "main node");

        DEBUG_PTR ((*node)->right);
        DEBUG_PTR (*node);
    }

    if (*curToken != tokens->size)
        SYNTAX_ERROR;

    return TREE_OK;
}


#define IS_TOKEN_KEYWORD(keyword)                        \
        (tokens->data[*curToken].type == TYPE_KEYWORD && \
         tokens->data[*curToken].value.idx == keyword)

int GetOperation (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (IS_TOKEN_KEYWORD (KEY_IF))
    {
        DEBUG_STR ("IF!!!!!!!!!");

        int status = GetIf (program, tokens, curToken, node);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        return TREE_OK;
    }

    if (IS_TOKEN_KEYWORD (KEY_OPEN_BRACKET))
    {
        (*curToken)++;

        int status = GetOperation (program, tokens, curToken, node);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        while (!IS_TOKEN_KEYWORD (KEY_CLOSE_BRACKET))
        {
            node_t *nodeArg = NULL;
            status = GetOperation (program, tokens, curToken, &nodeArg);
            if (status != TREE_OK)
                SYNTAX_ERROR;

            *node = CONNECT_ (*node, nodeArg);
        }

        (*curToken)++;

        return TREE_OK;
    }

    int status = GetAssign (program, tokens, curToken, node);
    if (status != TREE_OK)
        SYNTAX_ERROR;
    
    return TREE_OK;
}

int GetIf (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (!IS_TOKEN_KEYWORD (KEY_IF))
        SYNTAX_ERROR;

    (*curToken)++;

    if (!IS_TOKEN_KEYWORD (KEY_OPEN_PARENS))
        SYNTAX_ERROR;
    
    (*curToken)++;

    node_t *nodeL = NULL;
    int status = GetExpression (program, tokens, curToken, &nodeL);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    if (!IS_TOKEN_KEYWORD (KEY_CLOSE_PARENS))
        SYNTAX_ERROR;

    (*curToken)++;
    
    node_t *nodeR = NULL;
    status = GetOperation (program, tokens, curToken, &nodeR);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    *node = IF_ (nodeL, nodeR);

    NODE_DUMP (program, *node, "Created new node (if). curToken = %lu", *curToken);

    return TREE_OK;
}


int GetAssign (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);
    
    // if (strncmp ("MC ", *curPos, 3) != )

    node_t *nodeL = NULL;
    int status = GetVariable (program, tokens, curToken, &nodeL);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    if (!IS_TOKEN_KEYWORD (KEY_ASSIGN))
        SYNTAX_ERROR;

    (*curToken)++;

    node_t *nodeR = NULL;
    status = GetExpression (program, tokens, curToken, &nodeR);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    if (!IS_TOKEN_KEYWORD (KEY_CONNECT))
        SYNTAX_ERROR;
    
    (*curToken)++;
    
    *node = ASSIGN_ (nodeL, nodeR);

    NODE_DUMP (program, *node, "Created new node (assign). curToken = %lu", *curToken);

    return TREE_OK;
}

int GetExpression (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    int status = GetTerm (program, tokens, curToken, node);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    while (IS_TOKEN_KEYWORD(KEY_ADD) ||
           IS_TOKEN_KEYWORD(KEY_SUB))
    {
        size_t operation = tokens->data[*curToken].value.idx;

        (*curToken)++;
        
        node_t *node2 = NULL;
        status = GetTerm (program, tokens, curToken, &node2);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        switch (operation)
        {
            case KEY_ADD: *node = ADD_ (*node, node2); break;
            case KEY_SUB: *node = SUB_ (*node, node2); break;

            default: SYNTAX_ERROR;
        }
    }

    NODE_DUMP (program, *node, "Created new node (ast). curToken = %lu", *curToken);

    return TREE_OK;
}

int GetTerm (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    int status = GetPower (program, tokens, curToken, node);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    while (IS_TOKEN_KEYWORD(KEY_MUL) ||
           IS_TOKEN_KEYWORD(KEY_DIV))
    {
        size_t operation = tokens->data[*curToken].value.idx;
        
        (*curToken)++;
        
        node_t *node2 = NULL;
        status = GetPower (program, tokens, curToken, &node2);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        switch (operation)
        {
            case KEY_MUL: *node = MUL_ (*node, node2); break;
            case KEY_DIV: *node = DIV_ (*node, node2); break;

            default: SYNTAX_ERROR;
        }
    }

    DEBUG_LOG ("node [%p]",  (*node));
    DEBUG_LOG ("\tnode->left [%p]",  (*node)->left);
    DEBUG_LOG ("\tnode->right [%p]", (*node)->right);

    NODE_DUMP (program, *node, "Created new node (term). curToken = %lu", *curToken);

    return TREE_OK;
}

int GetPower (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    int status = GetPrimaryExpression (program, tokens, curToken, node);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    while (IS_TOKEN_KEYWORD (KEY_POW))
    {
        (*curToken)++;
        
        node_t *node2 = NULL;
        status = GetPrimaryExpression (program, tokens, curToken, &node2);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        *node = POW_ (*node, node2);
    }

    return TREE_OK;
}

int GetPrimaryExpression (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (IS_TOKEN_KEYWORD (KEY_OPEN_PARENS))
    {
        (*curToken)++;
        
        int status = GetExpression (program, tokens, curToken, node);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        if (!IS_TOKEN_KEYWORD (KEY_CLOSE_PARENS))
            SYNTAX_ERROR;

        (*curToken)++;

        return status;
    }

    int status = GetNumber (program, tokens, curToken, node);
    if (status == TREE_OK)
        return status;

    status = GetFunction (program, tokens, curToken, node);
    if (status == TREE_OK)
        return status;
    
    status = GetVariable (program, tokens, curToken, node);
    DEBUG_LOG ("GetVariable() status = %d", status);

    if (status == TREE_OK)
        return status;
    
    SYNTAX_ERROR;
}

int GetNumber (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (tokens->data[*curToken].type != TYPE_CONST_NUM)
        return TREE_ERROR_INVALID_TOKEN;

    valueNumber_t val = tokens->data[*curToken].value.number;
    DEBUG_VAR (VALUE_NUMBER_FSTRING, val);

    *node = NUM_ (val);

    (*curToken)++;

    NODE_DUMP (program, *node, "Created new node (number). curToken = %lu", *curToken);

    return TREE_OK;
}


int GetFunction (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (tokens->data[*curToken].type != TYPE_KEYWORD)
        return TREE_ERROR_INVALID_TOKEN;

    const keyword_t *func = FindBuiltinFunctionByIdx ((keywordIdxes_t) tokens->data[*curToken].value.idx);

    if (func == NULL)
    {
        DEBUG_LOG ("%s", "No builtin function found. Return");

        return TREE_ERROR_INVALID_TOKEN;
    }
    
    if (func->numberOfArgs < 1 ||
        func->numberOfArgs > 2)
    {
        ERROR_LOG ("%s", "Where are builitn functions with only 1 or 2 args now...\n"
                         "Maybe you forgot to rewrite this part of code?");

        return TREE_ERROR_INVALID_TOKEN;
    }

    if (!IS_TOKEN_KEYWORD (KEY_OPEN_PARENS))
        SYNTAX_ERROR;
    
    (*curToken)++;

    node_t *firstArg = {};
    int status = GetExpression (program, tokens, curToken, &firstArg);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    if (func->numberOfArgs == 1)
    {
        if (IS_TOKEN_KEYWORD (KEY_CLOSE_PARENS))
            SYNTAX_ERROR;
        
        (*curToken)++;
        (*node)->right = firstArg;

        return TREE_OK;
    }
    
    if (!IS_TOKEN_KEYWORD (KEY_COMMA))
        SYNTAX_ERROR;
    
    (*curToken)++;

    node_t *secondArg = {};
    status = GetExpression (program, tokens, curToken, &secondArg);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    if (!IS_TOKEN_KEYWORD (KEY_CLOSE_PARENS))
        SYNTAX_ERROR;

    (*curToken)++;

    (*node)->left  = firstArg;
    (*node)->right = secondArg;

    return TREE_OK;
}

#undef IS_TOKEN_KEYWORD

int GetVariable (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (tokens->data[*curToken].type != TYPE_VARIABLE)
        return TREE_ERROR_INVALID_TOKEN;
    
    // char *varName = *curPos;
    // DEBUG_VAR ("%p", varName);

    // int res = GetVariableName (curPos);
    // DEBUG_VAR ("%p", *curPos);

    // if (res != TREE_OK)
    // {
    //     DEBUG_LOG ("'%s' not a variable", *curPos);

    //     return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    // }

    // type_t type   = {};
    // value_t value = {};
    
    // size_t varNameLen = size_t (*curPos - varName);
    // DEBUG_VAR ("%lu", varNameLen);

    // int status = FindOrAddVariable (program, &varName, varNameLen, &type, &value);
    // if (status != TREE_OK)
    //     return status;

    *node = NodeCtorAndFill (&program->ast, tokens->data[*curToken].type, tokens->data[*curToken].value, NULL, NULL);

    NODE_DUMP (program, *node, "Created new node (variable). curToken = %lu", *curToken);

    (*curToken)++;

    return TREE_OK;
}

// TODO: delete
// int GetVariableName (char **curPos)
// {
//     assert (curPos);

//     if (!isalpha (**curPos) && **curPos != '_')
//         return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    
//     (*curToken)++;

//     DEBUG_VAR ("%p", *curPos);

//     while (isalpha (**curPos) || isdigit (**curPos) || **curPos == '_')
//     {
//         DEBUG_LOG ("**curPos(variable name) = \"%c\"", **curPos);
//         DEBUG_VAR ("%p", *curPos);
        
//         (*curPos)++;    
//     }
    
//     return TREE_OK;
// }

#include "dsl_undef.h"

#undef SYNTAX_ERROR