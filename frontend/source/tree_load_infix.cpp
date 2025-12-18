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
Declarate   ::= Variable  ":=" PrimaryExp ';'
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




Examples+ = 
{
"панч про_макана()
пошумим
    а стал 8 тррря
    раунд
воу

зачитать панч про_макана()"
}

Gramma              ::= {Function | Main}+ '\0'

Function            ::= "раунд" FuncName FuncCommon
Main                ::= "баттл" FuncName FuncCommon
FuncName            ::= Name
FuncCommon          ::= '(' ')'
                        "пошумим"
                            Operation*
                            "лучше_я_сдохну_чем_стану" Expression
                        "воу"

Operation           ::= {If | "пошумим" Opeartion+ "воу" | DeclaarteOrAssign | Print | Return | FunctionCall}  // FIXME
If                  ::= "биф"  '(' E ')' Operation 
Print               ::= "зачитать" '(' Expression ')' "тррря"
Return              ::= "лучше_я_сдохну_чем_стану" Expression
DeclarateOrAssign   ::= "мс " Variable  {"представься" | "стал"} Expression "тррря"

Expression          ::= Term        {['фит', 'дисс']        Term}* 
Term                ::= Power       {['антихайп', 'хайп']  Power}*
Power               ::= PrimaryExp  {'^'  PrimaryExp}* 
PrimaryExp          ::=  {'('  Expression ')' | Number | BuiltinFunction | FunctionCall | Variable } // FIXME

FunctionCall        ::= "зачитать" FuncName '(' ')'
Number              ::= ['0'-'9']+{'.'['0'-'9']+}?
BuiltinFunction     ::= "спросить" '('  ')' |
                        ["sin", "cos", ...]  '(' Expression ')' | // Not implemented now
                        ["log"]  '(' Expression ',' Expression')'

Variable            ::= Name
Name                ::= ['a'-'z', 'A'-'Z', '_']['a'-'z', 'A'-'Z', '0'-'9', '_']*

*/

#define SYNTAX_ERROR                                                    \
        do                                                              \
        {                                                               \
            ERROR_LOG ("%s:%lu:%lu Syntax Error with token [%lu]",      \
                       program->tokens.fileName,                        \
                       tokens->data[*curToken].line,                    \
                       tokens->data[*curToken].position,                \
                       *curToken);                                      \
                                                                        \
            return TREE_ERROR_SYNTAX_IN_SAVE_FILE;                      \
        }                                                               \
        while (0)

#define SYNTAX_ERROR_MESSAGE(format, ...)                                               \
        do                                                                              \
        {                                                                               \
            ERROR_PRINT ("%s:%lu:%lu Syntax Error with token [%lu] :\n" format "\n",    \
                         program->tokens.fileName,                                      \
                         tokens->data[*curToken].line,                                  \
                         tokens->data[*curToken].position,                              \
                         *curToken,                                                     \
                         __VA_ARGS__);                                                  \
                                                                                        \
            return TREE_ERROR_SYNTAX_IN_SAVE_FILE;                                      \
        }                                                                               \
        while (0)

static int GetGramma            (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetMain              (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetFunction          (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetFunctionCommon      (program_t *program, tokensArray_t *tokens,
                                 size_t *curToken, node_t **node);
static int GetFunctionCall      (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int AddFunctionName      (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetOperation         (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetIf                (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
// static int GetAssign            (program_t *program, tokensArray_t *tokens, 
//                                  size_t *curToken, node_t **node);
static int GetDeclarateOrAssign (program_t *program, tokensArray_t *tokens, 
                                size_t *curToken, node_t **node);
static int GetPrint             (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
static int GetReturn            (program_t *program, tokensArray_t *tokens, 
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
                                 size_t *curToken, node_t **node, 
                                 bool reportErrors);
static int SetVariable          (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node, 
                                 bool reportErrors);
static int GetBuiltinFunction   (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);
// static int GetVariableName      (char **curPos);
static int GetNumber            (program_t *program, tokensArray_t *tokens, 
                                 size_t *curToken, node_t **node);

int TreeLoadInfixFromTokens (program_t *program)
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

    int status = GetFunction (program, tokens, curToken, node);
    if (status != TREE_OK)
        SYNTAX_ERROR_MESSAGE ("%s", "Бро, почему у тебя пустая программа? Где рэпчик?");

    *node = CONNECT_ (*node, NULL);
    NODE_DUMP (program, *node, "Created new node (connection). curToken = %lu", *curToken);

    while (*curToken < tokens->size)
    {
        node_t *nodeNext = NULL;

        status = GetMain (program, tokens, curToken, &nodeNext);
        
        if (status != TREE_OK)
            status = GetFunction (program, tokens, curToken, &nodeNext);

        if (status != TREE_OK)
            SYNTAX_ERROR_MESSAGE ("%s", "Ресторатор недоволен");

        // (*node)->right = CONNECT_ (nodeNext, NULL);
        // *node = (*node)->right;
        
        *node = CONNECT_ (*node, nodeNext);
        NODE_DUMP (program, (*node), "Created new node (connection). curToken = %lu", *curToken);
        // NODE_DUMP (program, (*node), "%s", "main node");

        DEBUG_PTR ((*node)->right);
        DEBUG_PTR (*node);
    }

    if (*curToken != tokens->size)
        SYNTAX_ERROR_MESSAGE ("%s", "Что-то не сходится у меня...");

    return TREE_OK;
}

#define IS_NEXT_TOKEN_KEYWORD(keyword)                       \
        (*curToken + 1 <= tokens->size &&                    \
         tokens->data[*curToken + 1].type == TYPE_KEYWORD && \
         tokens->data[*curToken + 1].value.idx == keyword)

#define IS_TOKEN_KEYWORD(keyword)                        \
        (*curToken <= tokens->size &&                    \
         tokens->data[*curToken].type == TYPE_KEYWORD && \
         tokens->data[*curToken].value.idx == keyword)

#define IS_TOKEN_TYPE(tokenType)                    \
        (*curToken <= tokens->size &&               \
         tokens->data[*curToken].type == tokenType)


int GetMain (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (!IS_TOKEN_KEYWORD (KEY_MAIN))
        return TREE_ERROR_INVALID_TOKEN;
    
    (*curToken)++;

    *node = MAIN_ (NULL, NULL);
    NODE_DUMP (program, (*node), "Created new node (func). curToken = %lu", *curToken);

    TREE_DO_AND_RETURN (AddFunctionName (program, tokens, curToken, &(*node)->left));

    return GetFunctionCommon (program, tokens, curToken, node);
}

int GetFunction (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (!IS_TOKEN_KEYWORD (KEY_FUNC))
        SYNTAX_ERROR_MESSAGE ("%s", "Bro, язык состоит только из функций, врубаешься?\n"
                                    "Это нормальный язык, а не петухон какой-то\n"
                                    "В следующий раз нормально читай рэп без этой фигни");
    
    (*curToken)++;

    *node = FUNC_ (NULL, NULL);
    NODE_DUMP (program, (*node), "Created new node (func). curToken = %lu", *curToken);

    (*node)->left =  COMMA_ (NULL, NULL);

    TREE_DO_AND_RETURN (AddFunctionName (program, tokens, curToken, &((*node)->left->left)));

    return GetFunctionCommon (program, tokens, curToken, node);
}

int GetFunctionCommon (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (!IS_TOKEN_KEYWORD (KEY_OPEN_PARENS))
        SYNTAX_ERROR_MESSAGE ("%s", "А скобки за тебя Пушкин* ставить будет?\n"
                                    "* - старейший баттл рэпер России");

    (*curToken)++;

    if (!IS_TOKEN_KEYWORD (KEY_CLOSE_PARENS))
        SYNTAX_ERROR_MESSAGE ("%s", "А скобки за тебя Пушкин* закрывть будет?\n"
                                    "* - старейший баттл рэпер России");

    (*curToken)++;

    if (!IS_TOKEN_KEYWORD (KEY_OPEN_BRACKET))
        SYNTAX_ERROR_MESSAGE ("%s", "А что зал такой тухлый? Где \"шум\" aka '{'?");

    (*curToken)++;
    
    while (!IS_TOKEN_KEYWORD (KEY_RETURN))
    {
        node_t *nodeNext = NULL;

        int status = GetOperation (program, tokens, curToken, &nodeNext);
        if (status != TREE_OK)
            SYNTAX_ERROR_MESSAGE ("%s", "Бро, что-то слабый раунд, разберись с телом функции");

        (*node)->right = CONNECT_ ((*node)->right, nodeNext);
        NODE_DUMP (program, (*node), "Created new node (connection). curToken = %lu", *curToken);
    }

    DumpTokens (program);

    if (!IS_TOKEN_KEYWORD (KEY_RETURN))
        SYNTAX_ERROR_MESSAGE ("%s", "А кто раунд завершать будет (где return)?");

    (*node)->right = CONNECT_ ((*node)->right, NULL);
    NODE_DUMP (program, (*node), "Created new node(connection) before adding return. curToken = %lu", *curToken);

    node_t *body = (*node)->right; 
    TREE_DO_AND_RETURN (GetReturn (program, tokens, curToken, &(body->right)));

    if (!IS_TOKEN_KEYWORD (KEY_CLOSE_BRACKET))
        SYNTAX_ERROR_MESSAGE ("%s", "А что зал такой тухлый? Где \"воу\" aka '}' ?");

    (*curToken)++;

    return TREE_OK;
}

int GetFunctionCall (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (!IS_TOKEN_KEYWORD (KEY_CALL))
        return TREE_ERROR_INVALID_TOKEN;

    (*curToken)++;

    if (!IS_TOKEN_TYPE (TYPE_NAME))
        SYNTAX_ERROR;

    const name_t *funcName = NamesTableFindByIdx (&program->namesTable, tokens->data[*curToken].value.idx);
    if (funcName == NULL)
        SYNTAX_ERROR_MESSAGE ("%s", "Чувак, ты пытаешься зачитать ещё ненаписанный раунд...");
    // FIXME: добавить в NamesTable параметры func или var
    // Ошибка в том, что можно вызвать перменную
    // А ещё можно вызвать функцию ниже, в отличии от решения с поиском по стеку
    // короче думайте
    

    return TREE_OK;
}

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

    int status = GetDeclarateOrAssign (program, tokens, curToken, node);
    if (status == TREE_OK)
        return TREE_OK;

    DEBUG_LOG ("%s", "Token isn't Declarartion or assignment");

    status = GetReturn (program, tokens, curToken, node);
    if (status == TREE_OK)
        return TREE_OK;

    DEBUG_LOG ("%s", "Token isn't Return or assignment");


    status = GetPrint (program, tokens, curToken, node);
    if (status != TREE_OK)
        SYNTAX_ERROR_MESSAGE ("%s", "Неизвестная операция\n");
    
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


// int GetAssign (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
// {
//     assert (program);
//     assert (tokens);
//     assert (curToken);
//     assert (node);
    
//     // if (strncmp ("MC ", *curPos, 3) != )

//     node_t *nodeL = NULL;
// DEBUG_VAR ("%lu", *curToken);

//     int status = GetVariable (program, tokens, curToken, &nodeL, true);
//     if (status != TREE_OK)
//         SYNTAX_ERROR;
        
//     DEBUG_VAR ("%lu", *curToken);

//     if (!IS_TOKEN_KEYWORD (KEY_ASSIGN))
//         SYNTAX_ERROR;

//     (*curToken)++;

//     node_t *nodeR = NULL;
//     status = GetExpression (program, tokens, curToken, &nodeR);
//     if (status != TREE_OK)
//         SYNTAX_ERROR;

//     if (!IS_TOKEN_KEYWORD (KEY_CONNECT))
//         SYNTAX_ERROR;
    
//     (*curToken)++;
    
//     *node = ASSIGN_ (nodeL, nodeR);

//     NODE_DUMP (program, *node, "Created new node (assign). curToken = %lu", *curToken);

//     return TREE_OK;
// }


int GetDeclarateOrAssign (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    node_t *nodeL = NULL;

    if (IS_NEXT_TOKEN_KEYWORD (KEY_DECLARATE))
    {
        TREE_DO_AND_RETURN (SetVariable (program, tokens, curToken, &nodeL, true));
    }
    else if (IS_NEXT_TOKEN_KEYWORD (KEY_ASSIGN))
    {
        TREE_DO_AND_RETURN (GetVariable (program, tokens, curToken, &nodeL, true));
    }
    else
    {
        return TREE_ERROR_INVALID_TOKEN;
    }

    size_t idx = tokens->data[*curToken].value.idx;
    
    (*curToken)++;

    node_t *nodeR = NULL;
    int status = GetExpression (program, tokens, curToken, &nodeR);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    DEBUG_VAR ("%lu", *curToken);

    if (!IS_TOKEN_KEYWORD (KEY_CONNECT))
        SYNTAX_ERROR;
    
    (*curToken)++;
    
    *node = NodeCtorAndFill (&program->ast, 
                             TYPE_KEYWORD, {.idx = idx},
                             nodeL, nodeR);

    NODE_DUMP (program, *node, "Created new node (declaration). curToken = %lu", *curToken);

    return TREE_OK;
}

int GetPrint (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (!IS_TOKEN_KEYWORD (KEY_PRINT))
        return TREE_ERROR_INVALID_TOKEN;

    (*curToken)++;

    if (!IS_TOKEN_KEYWORD (KEY_OPEN_PARENS))
        SYNTAX_ERROR;

    (*curToken)++;

    *node = PRINT_ (NULL, NULL);

    int status = GetExpression (program, tokens, curToken, &(*node)->left);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    if (!IS_TOKEN_KEYWORD (KEY_CLOSE_PARENS))
        SYNTAX_ERROR;

    (*curToken)++;

    if (!IS_TOKEN_KEYWORD (KEY_CONNECT))
    {
        ERROR_LOG ("%s", "Missing \"тррря\" ");

        SYNTAX_ERROR;
    }

    (*curToken)++;

    *node = CONNECT_ (*node, NULL);

    return TREE_OK;
}


int GetReturn (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (!IS_TOKEN_KEYWORD (KEY_RETURN))
        return TREE_ERROR_INVALID_TOKEN;

    (*curToken)++;

    (*node) = (RETURN_ (NULL, NULL));

    int status = GetExpression (program, tokens, curToken, &(*node)->left);
    if (status != TREE_OK)
        SYNTAX_ERROR_MESSAGE ("%s", "Ошибка в выражении у return'a");

    (*node) = (CONNECT_ (*node, NULL));

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

    // (*curToken)++; // FIXME: I just removed this at 23:15 15 dec

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

    status = GetBuiltinFunction (program, tokens, curToken, node);
    if (status == TREE_OK)
        return status;
    
    status = GetVariable (program, tokens, curToken, node, true);
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

    NODE_DUMP (program, *node, "Created new node (number). curToken = %lu", *curToken);
    
    (*curToken)++;

    return TREE_OK;
}


int GetBuiltinFunction (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
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
        DEBUG_LOG ("No builtin function found by idx %lu. Return", tokens->data[*curToken].value.idx);

        return TREE_ERROR_INVALID_TOKEN;
    }
    
    if (func->numberOfArgs > 2)
    {
        ERROR_LOG ("%s", "Where are builitn functions with only 0, 1 or 2 args now...\n"
                         "Maybe you forgot to rewrite this part of code?");

        return TREE_ERROR_INVALID_TOKEN;
    }

    *node = NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = (size_t) func->idx}, NULL, NULL);

    (*curToken)++;

    if (!IS_TOKEN_KEYWORD (KEY_OPEN_PARENS))
        SYNTAX_ERROR;
    
    (*curToken)++;

    if (func->numberOfArgs == 0)
    {
        if (!IS_TOKEN_KEYWORD (KEY_CLOSE_PARENS))
            SYNTAX_ERROR;

        (*curToken)++;

        return TREE_OK;
    }

    node_t *firstArg = {};
    int status = GetExpression (program, tokens, curToken, &firstArg);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    if (func->numberOfArgs == 1)
    {
        if (!IS_TOKEN_KEYWORD (KEY_CLOSE_PARENS))
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


int GetVariable (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node, 
                 bool reportErrors)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (!IS_TOKEN_TYPE (TYPE_NAME))
    {
        if (reportErrors)
            ERROR_LOG ("Token number [%lu] doesn't have string type", *curToken);

        DumpTokens (program);

        return TREE_ERROR_INVALID_TOKEN;
    }

    const token_t *token = &tokens->data[*curToken];

    const name_t *var = NamesTableFindByIdx (&program->namesTable, token->value.idx);
    if (var == NULL)
    {
        SYNTAX_ERROR_MESSAGE ("%s", "Uknown variable");
    }

    if (StackFind (&program->variables, token->value.idx) != STACK_OK)
        SYNTAX_ERROR_MESSAGE ("Variable \"%.*s\" used, but not declarated before", (int) var->len, var->name);

    *node = NodeCtorAndFill (&program->ast, token->type, token->value, NULL, NULL);

    NODE_DUMP (program, *node, "Created new node (variable). curToken = %lu", *curToken);

    (*curToken)++;

    return TREE_OK;
}


int SetVariable (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node, 
                 bool reportErrors)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (!IS_TOKEN_TYPE (TYPE_NAME))
    {
        if (reportErrors)
            ERROR_LOG ("Token number [%lu] doesn't have TYPE_NAME", *curToken);

        DumpTokens (program);

        return TREE_ERROR_INVALID_TOKEN;
    }

    const token_t *token = &tokens->data[*curToken];

    const name_t *var = NamesTableFindByIdx (&program->namesTable, token->value.idx);
    if (var == NULL)
    {
        ERROR_LOG ("%s", "Uknown variable name, error somewhere in tokenizator");
        SYNTAX_ERROR;
    }

    if (StackFind (&program->variables, token->value.idx) == STACK_OK)
    {
        ERROR_LOG ("Redeclaration of the variable \"%.*s\"", (int) var->len, var->name);
        SYNTAX_ERROR;
    }

    int status = StackPush (&program->variables, token->value.idx);
    if (status != STACK_OK)
    {
        StackPrintError (status);
        
        return TREE_ERROR_STACK |
               status;
    }

    *node = NodeCtorAndFill (&program->ast, token->type, token->value, NULL, NULL);

    NODE_DUMP (program, *node, "Created new node (variable). curToken = %lu", *curToken);

    (*curToken)++;

    return TREE_OK;
}

int AddFunctionName (program_t *program, tokensArray_t *tokens, size_t *curToken, node_t **node)
{
    assert (program);
    assert (tokens);
    assert (curToken);
    assert (node);

    if (!IS_TOKEN_TYPE (TYPE_NAME))
    {
        ERROR_LOG ("Token number [%lu] doesn't have type TYPE_NAME", *curToken);
        
        DumpTokens (program);

        return TREE_ERROR_INVALID_TOKEN;
    }

    const token_t *token = &tokens->data[*curToken];

    const name_t *var = NamesTableFindByIdx (&program->namesTable, token->value.idx);
    if (var == NULL)
    {
        ERROR_LOG ("%s", "Uknown function name, error somewhere in tokenizator");
        SYNTAX_ERROR;
    }
    
    if (StackFind (&program->functions, token->value.idx) == STACK_OK)
    {
        ERROR_LOG ("Redeclaration of the function \"%.*s\"", (int) var->len, var->name);
        SYNTAX_ERROR;
    }

    int status = StackPush (&program->variables, token->value.idx);
    if (status != STACK_OK)
    {
        StackPrintError (status);
        
        return TREE_ERROR_STACK |
               status;
    }

    *node = NAME_ (token->value.idx);

    NODE_DUMP (program, *node, "Created new node (variable). curToken = %lu", *curToken);

    (*curToken)++;

    return TREE_OK;
}

#include "dsl_undef.h"

#undef SYNTAX_ERROR

#undef IS_NEXT_TOKEN_KEYWORD
#undef IS_TOKEN_KEYWORD
#undef IS_TOKEN_TYPE