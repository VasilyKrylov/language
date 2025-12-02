#ifndef K_TREE_AST_H
#define K_TREE_AST_H

#include <stdio.h>

#include "tree.h"

// : size_t because of size_t idx in variable_t
enum keywordIdxes_t : size_t
{
    OP_UKNOWN,
    OP_CONNECT,
    OP_ASSIGN,
    OP_IF,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_POW,
    OP_LOG,
    OP_LN,
    OP_SIN,
    OP_COS,
    OP_TG,
    OP_CTG,
    OP_ARCSIN,
    OP_ARCCOS,
    OP_ARCTG,
    OP_ARCCTG,
    OP_SH,
    OP_CH,
    OP_TH,
    OP_CTH,
};

struct variable_t
{
    char *name   = NULL;
    size_t len   = 0;

    size_t idx   = 0;
    double value = 0;
};

struct program_t
{
    treeLog_t log = {};

    tree_t expression = {};

    variable_t *variables = NULL;
    size_t variablesCapacity = 0;
    size_t variablesSize = 0;

    char *buffer = NULL;
};

struct keyword_t
{
    const char *name     = NULL;
    size_t nameLen       = 0;
    keywordIdxes_t idx     = OP_UKNOWN;
    bool isFunction      = 0;
    size_t numberOfArgs  = 0;
};

#define KEYWORD(nameStr, idxKey, isFunctionKey, numberOfArgsKey)    \
        {.name = nameStr,                                           \
         .nameLen = sizeof (nameStr) - 1,                           \
         .idx = idxKey,                                             \
         .isFunction = isFunctionKey,                               \
         .numberOfArgs = numberOfArgsKey}

const keyword_t keywords[] = 
{
    KEYWORD ("uknown_keyword",  OP_UKNOWN,  0,  0),
    KEYWORD ("some_connection", OP_CONNECT, 0,  0),
    KEYWORD ("=",               OP_ASSIGN,  0,  0),
    KEYWORD ("if",              OP_IF,      0,  0),
    KEYWORD ("+",               OP_ADD,     0,  0),
    KEYWORD ("-",               OP_SUB,     0,  0),
    KEYWORD ("*",               OP_MUL,     0,  0),
    KEYWORD ("/",               OP_DIV,     0,  0),
    KEYWORD ("^",               OP_POW,     0,  0),
    KEYWORD ("log",             OP_LOG,     1,  2),
    KEYWORD ("ln",              OP_LN,      1,  1),
    KEYWORD ("sin",             OP_SIN,     1,  1),
    KEYWORD ("cos",             OP_COS,     1,  1),
    KEYWORD ("tg",              OP_TG,      1,  1),
    KEYWORD ("ctg",             OP_CTG,     1,  1),
    KEYWORD ("arcsin",          OP_ARCSIN,  1,  1),
    KEYWORD ("arccos",          OP_ARCCOS,  1,  1),
    KEYWORD ("arctg",           OP_ARCTG,   1,  1),
    KEYWORD ("arcctg",          OP_ARCCTG,  1,  1),
    KEYWORD ("sh",              OP_SH,      1,  1),
    KEYWORD ("ch",              OP_CH,      1,  1),
    KEYWORD ("th",              OP_TH,      1,  1),
    KEYWORD ("cth",             OP_CTH,     1,  1),
};
const size_t kNumberOfKeywords = sizeof(keywords) / sizeof(keyword_t);


int ProgramCtor                     (program_t *program, size_t variablesCapacity);
void ProgramDtor                    (program_t *program);

const char *GetTypeName             (type_t type);

variable_t *FindVariableByIdx       (program_t *program, size_t idx);
variable_t *FindVariableByName      (program_t *program, char *varName, size_t varNameLen);
const keyword_t *FindKeywordByIdx   (keywordIdxes_t idx);

int CheckForReallocVariables        (program_t *program);
int FindOrAddVariable               (program_t *program, char **curPos, size_t len, 
                                     type_t *type, treeDataType *value);
void TryToFindOperator              (char *str, int len, type_t *type, treeDataType *value);

// int TreeSaveToFile               (tree_t *tree, const char *fileName);
// int NodeSaveToFile               (node_t *node, FILE *file);

int TreeCalculate                   (program_t *program, tree_t *expression);
double NodeCalculate                (program_t *program, node_t *node);

void TreeSimplify                   (program_t *program, tree_t *tree);

#endif // K_TREE_AST_H