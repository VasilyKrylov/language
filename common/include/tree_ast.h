#ifndef K_TREE_AST_H
#define K_TREE_AST_H

#include <stdio.h>

#include "tree.h"
#include "stack.h"

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
    KEY_INPUT,
    KEY_PRINT,
    KEY_CONNECT,
    KEY_COMMA,
    // KEY_INT,
    KEY_DECLARATE,
    KEY_ASSIGN,
    KEY_IF,
    KEY_OPEN_PARENS,
    KEY_CLOSE_PARENS,
    KEY_OPEN_BRACKET,
    KEY_CLOSE_BRACKET,
    KEY_FUNC,
    KEY_MAIN,
    KEY_RETURN,
    KEY_CALL,
};

struct token_t
{
    type_t type = TYPE_UKNOWN;
    value_t value = {.idx = KEY_UKNOWN};

    size_t line = 1;
    size_t position = 1;
};
struct tokensArray_t
{
    token_t *data = NULL;

    size_t size = 0;
    size_t capacity = 0;

    const char *fileName = NULL;
}; // TODO: array Ctor and make it universal on void*

struct name_t
{
    char *name   = NULL;
    size_t len   = 0;

    size_t idx   = 0;
};
struct namesTable_t
{
    name_t *data = NULL;

    size_t size = 0;
    size_t capacity = 0;
};

struct program_t
{
    treeLog_t log = {};

    tree_t ast = {};

    namesTable_t namesTable = {};

    stack_t variables = {};
    stack_t functions = {};

    tokensArray_t tokens = {};

    char *buffer   = NULL;
};

struct keyword_t
{
    const char *name            = NULL;
    const char *standardName    = NULL;
    size_t nameLen              = 0;
    keywordIdxes_t idx          = KEY_UKNOWN;
    bool isFunction             = 0;
    size_t numberOfArgs         = 0;
};

#define KEYWORD(nameKey, stndatdNameKey, idxKey, isFunctionKey, numberOfArgsKey)    \
        {.name          = nameKey,                                                  \
         .standardName  = stndatdNameKey,                                           \
         .nameLen       = sizeof (nameKey) - 1,                                     \
         .idx           = idxKey,                                                   \
         .isFunction    = isFunctionKey,                                            \
         .numberOfArgs  = numberOfArgsKey}

const keyword_t kKeywords[] = 
{
    KEYWORD ("хз",                       "uknown",      KEY_UKNOWN,         0,  0),
    KEYWORD ("фит",                      "+",           KEY_ADD,            0,  2),
    KEYWORD ("дисс",                     "-",           KEY_SUB,            0,  2),
    KEYWORD ("хайп",                     "*",           KEY_MUL,            0,  2),
    KEYWORD ("антихайп",                 "/",           KEY_DIV,            0,  2),
    KEYWORD ("TODO",                     "^",           KEY_POW,            0,  2),
    KEYWORD ("TODO",                     "log",         KEY_LOG,            1,  2),
    KEYWORD ("TODO",                     "ln",          KEY_LN,             1,  1),
    KEYWORD ("TODO",                     "sin",         KEY_SIN,            1,  1),
    KEYWORD ("TODO",                     "cos",         KEY_COS,            1,  1),
    KEYWORD ("TODO",                     "tg",          KEY_TG,             1,  1),
    KEYWORD ("TODO",                     "ctg",         KEY_CTG,            1,  1),
    KEYWORD ("TODO",                     "arcsin",      KEY_ARCSIN,         1,  1),
    KEYWORD ("TODO",                     "arccos",      KEY_ARCCOS,         1,  1),
    KEYWORD ("TODO",                     "arctg",       KEY_ARCTG,          1,  1),
    KEYWORD ("TODO",                     "arcctg",      KEY_ARCCTG,         1,  1),
    KEYWORD ("TODO",                     "sh",          KEY_SH,             1,  1),
    KEYWORD ("TODO",                     "ch",          KEY_CH,             1,  1),
    KEYWORD ("TODO",                     "th",          KEY_TH,             1,  1),
    KEYWORD ("TODO",                     "cth",         KEY_CTH,            1,  1),
    KEYWORD ("спросить",                 "input",       KEY_INPUT,          1,  0),
    KEYWORD ("панчлайн",                 "print",       KEY_PRINT,          1,  1),
    KEYWORD ("тррря",                    ";",           KEY_CONNECT,        0,  0),
    KEYWORD ("TODO",                     ",",           KEY_COMMA,          0,  0),
    // KEYWORD ("мс",                       "NOT_IN_AST",  KEY_INT,            0,  0),
    KEYWORD ("представься",              ":=",          KEY_DECLARATE,      0,  0),
    KEYWORD ("стал",                     "=",           KEY_ASSIGN,         0,  0),
    KEYWORD ("биф",                      "if",          KEY_IF,             0,  0),
    KEYWORD ("(",                        "(",           KEY_OPEN_PARENS,    0,  0),
    KEYWORD (")",                        ")",           KEY_CLOSE_PARENS,   0,  0),
    KEYWORD ("пошумим",                  "{",           KEY_OPEN_BRACKET,   0,  0),
    KEYWORD ("воу",                      "}",           KEY_CLOSE_BRACKET,  0,  0),
    KEYWORD ("раунд",                    "func",        KEY_FUNC,           0,  0),
    KEYWORD ("баттл",                    "main",        KEY_MAIN,           0,  0),
    KEYWORD ("лучше_я_сдохну_чем_стану", "return",      KEY_RETURN,         1,  1),
    KEYWORD ("зачитать" ,                "call",        KEY_CALL,           0,  0),
};
const size_t kNumberOfKeywords = sizeof(kKeywords) / sizeof(keyword_t);


int ProgramCtor     (program_t *program);
void ProgramDtor    (program_t *program);

int NamesTableCtor  (namesTable_t *namesTable);
void NamesTableDtor (namesTable_t *namesTable);

const char *GetTypeName (type_t type);

const name_t *NamesTableFindByIdx (namesTable_t *namesTable, size_t idx);
const name_t *NamesTableFindByStr (namesTable_t *namesTable, char *varName, size_t varNameLen);
const keyword_t *FindKeywordByIdx (keywordIdxes_t idx);

const keyword_t *FindBuiltinFunctionByIdx (keywordIdxes_t idx);

int CheckForReallocNamesTable (namesTable_t *namesTable);

int NamesTableFindOrAdd (namesTable_t *namesTable, char *varName, size_t len, 
                         size_t *idx);

void TryToFindOperator (char *str, int len, type_t *type, treeDataType *value);
void TryToFindNode     (char *str, int len, type_t *type, treeDataType *value);

int TreeAstSaveToFile  (program_t *program, const char *fileName);
int PrintNode          (FILE *file, program_t *program, node_t *node, bool exitQuotes);

int TreeCalculate      (program_t *program, tree_t *ast);
double NodeCalculate   (program_t *program, node_t *node);

void TreeSimplify      (program_t *program, tree_t *tree);

void NamesTableDump    (namesTable_t *namesTable);

#endif // K_TREE_AST_H