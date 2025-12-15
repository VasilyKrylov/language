#ifndef K_TREE_AST_H
#define K_TREE_AST_H

#include <stdio.h>

#include "tree.h"

enum keywordIdxes_t
{
    KEY_UKNOWN,
    KEY_ADD,
    KEY_SUB,
    KEY_MUL,
    KEY_DIV,
    KEY_POW,
    KEY_LOG,
    KEY_LN,
    KEY_SIN,
    KEY_COS,
    KEY_TG,
    KEY_CTG,
    KEY_ARCSIN,
    KEY_ARCCOS,
    KEY_ARCTG,
    KEY_ARCCTG,
    KEY_SH,
    KEY_CH,
    KEY_TH,
    KEY_CTH,
    KEY_CONNECT,
    KEY_COMMA,
    KEY_DECLARATION,
    KEY_ASSIGN,
    KEY_IF,
    KEY_OPEN_PARENS,
    KEY_CLOSE_PARENS,
    KEY_OPEN_BRACKET,
    KEY_CLOSE_BRACKET,
};

struct token_t
{
    type_t type = TYPE_UKNOWN;
    value_t value = {.idx = KEY_UKNOWN};

    size_t line = 0;
    size_t position = 0;
};
struct tokensArray_t
{
    token_t *data = NULL;

    size_t size = 0;
    size_t capacity = 0;
}; // TODO: array Ctor and make it universal on void*

struct variable_t
{
    char *name   = NULL;
    size_t len   = 0;

    size_t idx   = 0;
};
struct variablesTable_t
{
    variable_t *data = NULL;

    size_t size = 0;
    size_t capacity = 0;
};

struct program_t
{
    treeLog_t log = {};

    tree_t ast = {};

    variablesTable_t variables = {};
    tokensArray_t tokens = {};

    char *buffer = NULL;
};

struct keyword_t
{
    const char *name     = NULL;
    size_t nameLen       = 0;
    keywordIdxes_t idx   = KEY_UKNOWN;
    bool isFunction      = 0;
    size_t numberOfArgs  = 0;
};

#define KEYWORD(nameStr, idxKey, isFunctionKey, numberOfArgsKey)    \
        {.name = nameStr,                                           \
         .nameLen = sizeof (nameStr) - 1,                           \
         .idx = idxKey,                                             \
         .isFunction = isFunctionKey,                               \
         .numberOfArgs = numberOfArgsKey}

const keyword_t kKeywords[] = 
{
    KEYWORD ("uknown_keyword",  KEY_UKNOWN,         0,  0),
    KEYWORD ("+",               KEY_ADD,            0,  0),
    KEYWORD ("-",               KEY_SUB,            0,  0),
    KEYWORD ("*",               KEY_MUL,            0,  0),
    KEYWORD ("/",               KEY_DIV,            0,  0),
    KEYWORD ("^",               KEY_POW,            0,  0),
    KEYWORD ("log",             KEY_LOG,            1,  2),
    KEYWORD ("ln",              KEY_LN,             1,  1),
    KEYWORD ("sin",             KEY_SIN,            1,  1),
    KEYWORD ("cos",             KEY_COS,            1,  1),
    KEYWORD ("tg",              KEY_TG,             1,  1),
    KEYWORD ("ctg",             KEY_CTG,            1,  1),
    KEYWORD ("arcsin",          KEY_ARCSIN,         1,  1),
    KEYWORD ("arccos",          KEY_ARCCOS,         1,  1),
    KEYWORD ("arctg",           KEY_ARCTG,          1,  1),
    KEYWORD ("arcctg",          KEY_ARCCTG,         1,  1),
    KEYWORD ("sh",              KEY_SH,             1,  1),
    KEYWORD ("ch",              KEY_CH,             1,  1),
    KEYWORD ("th",              KEY_TH,             1,  1),
    KEYWORD ("cth",             KEY_CTH,            1,  1),
    KEYWORD (";",               KEY_CONNECT,        0,  0),
    KEYWORD (",",               KEY_COMMA,          0,  0),
    KEYWORD (":=",              KEY_DECLARATION,    0,  0),
    KEYWORD ("=",               KEY_ASSIGN,         0,  0),
    KEYWORD ("if",              KEY_IF,             0,  0),
    KEYWORD ("(",               KEY_OPEN_PARENS,    0,  0),
    KEYWORD (")",               KEY_CLOSE_PARENS,   0,  0),
    KEYWORD ("{",               KEY_OPEN_BRACKET,  0,  0),
    KEYWORD ("}",               KEY_CLOSE_BRACKET,   0,  0),
};
const size_t kNumberOfKeywords = sizeof(kKeywords) / sizeof(keyword_t);


int ProgramCtor                     (program_t *program);
void ProgramDtor                    (program_t *program);

const char *GetTypeName             (type_t type);

const variable_t *FindVariableByIdx     (variablesTable_t *variables, size_t idx);
const variable_t *FindVariableByName    (variablesTable_t *variables, char *varName, size_t varNameLen);
const keyword_t *FindKeywordByIdx       (keywordIdxes_t idx);

const keyword_t *FindBuiltinFunctionByIdx (keywordIdxes_t idx);

int FindOrAddVariable               (program_t *program, char **curPos, size_t len, 
                                     type_t *type, treeDataType *value);
void TryToFindOperator              (char *str, int len, type_t *type, treeDataType *value);

int TreeAstSaveToFile               (program_t *program, const char *fileName);
int PrintNode                       (FILE *file, program_t *program, node_t *node);


int TreeCalculate                   (program_t *program, tree_t *ast);
double NodeCalculate                (program_t *program, node_t *node);

void TreeSimplify                   (program_t *program, tree_t *tree);

#endif // K_TREE_AST_H