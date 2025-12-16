#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

#include "tree_ast.h"

#include "tree.h"
#include "tokenizator.h"
#include "utils.h"
#include "float_math.h"

// static double NodeCalculateDoMath       (node_t *node, double leftVal, double rightVal);

static node_t *NodeSimplifyCalc         (tree_t *tree, node_t *node, bool *modified);
static node_t *NodeSimplifyTrivial      (tree_t *tree, node_t *node, bool *modified);

int NodeSaveToFile (FILE *file, program_t *program, node_t *node);

void PrintTabsToFile (FILE *file, size_t n);

int ProgramCtor (program_t *program)
{
    assert (program);

    TREE_DO_AND_RETURN (LogCtor (&program->log));
    TREE_DO_AND_RETURN (VariablesTableCtor (&program->variables));
    TREE_DO_AND_RETURN (TokensArrayCtor (&program->tokens));

    program->buffer = NULL;

    TREE_DO_AND_RETURN (TREE_CTOR (&program->ast, &program->log));

    return TREE_OK;
}

void ProgramDtor (program_t *program)
{
    assert (program);

    LogDtor (&program->log);
    VariablesTableDtor (&program->variables);
    TokensArrayDtor (&program->tokens);

    if (program->ast.root != NULL)
        TreeDtor (&program->ast);

    free (program->buffer);
    program->buffer = NULL;
}

int TreeAstSaveToFile (program_t *program, const char *fileName)
{
    assert (program);
    assert (fileName);

    DEBUG_STR (fileName);

    FILE *outputFile = fopen (fileName, "w");
    if (outputFile == NULL)
    {
        ERROR_LOG ("Error opening file \"%s\"", fileName);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_OPENING_FILE;
    }

    DEBUG_PTR (outputFile);

    int status = NodeSaveToFile (outputFile, program, program->ast.root);

    fclose (outputFile);

    return status;
}

void PrintTabsToFile (FILE *file, size_t n)
{
    assert (file);

    while (n--)
        fprintf (file, "\t");
}

int NodeSaveToFile (FILE *file, program_t *program, node_t *node)
{
    assert (file);
    assert (program);
    assert (node);

    static size_t tabsCounter = 0;
    
    tabsCounter++;

    fprintf (file, "%s", "( ");

    TREE_DO_AND_RETURN (PrintNode (file, program, node, false));

    fprintf (file, "%s", "\n");
    PrintTabsToFile (file, tabsCounter);

    if (node->left != NULL)
        NodeSaveToFile (file, program, node->left);
    else
        fprintf (file, "%s", "nil");

    fprintf (file, "%s", "\n");
    PrintTabsToFile (file, tabsCounter);

    if (node->right != NULL)
        NodeSaveToFile (file, program, node->right);
    else
        fprintf (file, "%s", "nil");

    fprintf (file, "%s", "\n");

    tabsCounter--;
    PrintTabsToFile (file, tabsCounter);

    fprintf (file, "%s", ")");

    return TREE_OK;
}

int PrintNode (FILE *file, program_t *program, node_t *node, bool exitQuotes)
{
    assert (file);
    assert (program);
    assert (node);

    switch (node->type)
    {
        case TYPE_UKNOWN:           fprintf (file, "UKNOWN");                                  break;
        case TYPE_CONST_NUM:        fprintf (file, VALUE_NUMBER_FSTRING, node->value.number);  break;

        case TYPE_KEYWORD:          
        {
            const keyword_t *keyword = FindKeywordByIdx ((keywordIdxes_t) node->value.idx);
            
            if (keyword == NULL)
                fprintf (file, "NULL keyword");
            else
                fprintf (file, "%s", keyword->standardName);                  
            break;
        }
        case TYPE_VARIABLE:         
        {
            const variable_t *var = FindVariableByIdx (&program->variables, (size_t) node->value.idx);

            if (var == NULL)
                fprintf (file, "NULL variable");
            else
            {
                if (exitQuotes)
                    fprintf (file, "\\\"%.*s\\\"", (int)var->len, var->name);
                else
                    fprintf (file, "\"%.*s\"", (int)var->len, var->name);
            }
            break;
        }

        default:
            fprintf (file, "error");

            return TREE_ERROR_INVALID_NODE;
    }

    return TREE_OK;
}

const char *GetTypeName (type_t type)
{
    switch (type)
    {
        case TYPE_UKNOWN:           return "uknown";
        case TYPE_CONST_NUM:        return "number";
        case TYPE_KEYWORD:          return "keyword";
        case TYPE_VARIABLE:         return "variable";

        default:                    return "ERROR";
    }
}

void TryToFindOperator (char *str, int len, type_t *type, treeDataType *value)
{
    assert (str);

    for (size_t i = 0; i < kNumberOfKeywords; i++)
    {
        if (strncmp (str, kKeywords[i].name, (size_t) len) == 0)
        {
            *type = TYPE_KEYWORD;
            value->idx = kKeywords[i].idx;

            DEBUG_LOG ("FOUND \"%s\"", kKeywords[i].name);
        }
    }
}

void TryToFindNode (char *str, int len, type_t *type, treeDataType *value)
{
    assert (str);

    for (size_t i = 0; i < kNumberOfKeywords; i++)
    {
        if (strncmp (str, kKeywords[i].standardName, (size_t) len) == 0)
        {
            *type = TYPE_KEYWORD;
            value->idx = kKeywords[i].idx;

            DEBUG_LOG ("FOUND \"%s\"", kKeywords[i].name);
        }
    }
}

const variable_t *FindVariableByName (variablesTable_t *variables, char *varName, size_t varNameLen)
{
    assert (variables);
    assert (varName);

    DEBUG_LOG ("varName = \"%.*s\"", (int) varNameLen, varName);
    DEBUG_VAR ("%lu", varNameLen);

    for (size_t i = 0; i < variables->size; i++)
    {
        DEBUG_VAR ("%lu", i);
        DEBUG_PTR (variables->data[i].name);
        DEBUG_LOG ("variables[i].name = \"%.*s\"", 
                   (int)variables->data[i].len,
                   variables->data[i].name);
        
        if (strncmp (variables->data[i].name, varName, varNameLen) == 0)
            return &variables->data[i];
    }

    return NULL;
}

const variable_t *FindVariableByIdx (variablesTable_t *variables, size_t idx)
{
    assert (variables);

    for (size_t i = 0; i < variables->size; i++)
    {
        if (variables->data[i].idx == idx)
            return &variables->data[i];
    }

    return NULL;
}

const keyword_t *FindKeywordByIdx (keywordIdxes_t idx)
{
    for (size_t i = 0; i < kNumberOfKeywords; i++)
    {
        if (kKeywords[i].idx == idx)
            return &kKeywords[i];
    }

    return NULL;
}

const keyword_t *FindBuiltinFunctionByIdx (keywordIdxes_t idx)
{
    for (size_t i = 0; i < kNumberOfKeywords; i++)
    {
        if (kKeywords[i].idx == idx && kKeywords[i].isFunction)
            return &kKeywords[i];
    }

    return NULL;
}

// FIXME: copy paste
int CheckForReallocVariables (variablesTable_t *namesTable)
{
    assert (namesTable);

    if (namesTable->size < namesTable->capacity)
        return TREE_OK;

    if (namesTable->capacity == 0)
        namesTable->capacity = 1;

    variable_t *newData = (variable_t *) realloc (namesTable->data, 
                                                      namesTable->capacity * sizeof (variable_t));
    
    if (newData == NULL)
    {
        ERROR_LOG ("Error reallocating memory - %s", strerror (errno));

        return TREE_ERROR_COMMON |
               COMMON_ERROR_ALLOCATING_MEMORY;
    }

    namesTable->data = newData;

    return TREE_OK;
}

int FindOrAddVariable (variablesTable_t *variables, char *varName, size_t len, 
                       size_t *idx)
{
    assert (variables);
    assert (varName);
    assert (idx);

    DEBUG_VAR ("%lu", len);

    const variable_t *variable = FindVariableByName (variables, varName, len);

    if (variable != NULL)
    {
        *idx = variable->idx;

        return TREE_OK;
    }
    
    TREE_DO_AND_RETURN (CheckForReallocVariables (variables));

    *idx = size_t(variables->size);

    variables->data[*idx].name = varName;
    variables->data[*idx].len = len;
    variables->data[*idx].idx = *idx;

    DEBUG_LOG ("%.*s", (int)variables->data[*idx].len, variables->data[*idx].name);

    variables->size++;
    
    DEBUG_VAR ("%lu", variables->size);
    DEBUG_LOG ("variable name is '%.*s'", 
               (int)variables->data[variables->size].len, varName);

    DumpVariables (variables);

    return TREE_OK;
}


// // =============  CALCULATION   =============

// int TreeCalculate (program_t *program, tree_t *ast)
// {
//     assert (program);
//     assert (ast);

//     DEBUG_VAR ("%lu", program->variablesSize);
    
//     double result = NodeCalculate (program, ast->root);

//     PRINT ("Expression is equals to %g\n", result);

//     return TREE_OK;
// }

// double NodeCalculate (program_t *program, node_t *node)
// {
//     assert (program);
//     assert (node);

//     double leftVal  = NAN;
//     double rightVal = NAN;

//     if (node->left != NULL)
//         leftVal = NodeCalculate (program, node->left);

//     if (node->right != NULL)
//         rightVal = NodeCalculate (program, node->right);

//     switch (node->type)
//     {
//         case TYPE_UKNOWN:
//             ERROR_LOG ("%s", "Uknown node while calculating tree");

//             return TREE_ERROR_INVALID_NODE;
            
//         case TYPE_CONST_NUM:
//             return node->value.number;

//         case TYPE_KEYWORD:
//             return NodeCalculateDoMath (node, leftVal, rightVal);

//         case TYPE_VARIABLE:
//             return NodeGetVariable (program, node);

//         default:
//             assert (0 && "Add new case in NodeCalctulate or wtf bro");
//     }
// }

// double NodeCalculateDoMath (node_t *node, double leftVal, double rightVal)
// {
//     assert (node);

//     switch (node->value.idx)
//     {
//         case KEY_ADD:    return leftVal + rightVal;
//         case KEY_SUB:    return leftVal - rightVal;
//         case KEY_MUL:    return leftVal * rightVal;
//         case KEY_DIV:    return leftVal / rightVal;
//         case KEY_POW:    return pow (leftVal, rightVal);
//         case KEY_LOG:    return logWithBase (leftVal, rightVal);
//         case KEY_LN:     return log (rightVal);
//         case KEY_SIN:    return sin (rightVal);
//         case KEY_COS:    return cos (rightVal);
//         case KEY_TG:     return tan (rightVal);
//         case KEY_CTG:    return 1 / tan (rightVal);
//         case KEY_ARCSIN: return asin (rightVal);
//         case KEY_ARCCOS: return acos (rightVal);
//         case KEY_ARCTG:  return atan (rightVal);
//         case KEY_ARCCTG: return 1 / atan (rightVal);
//         case KEY_SH:     return sinh (rightVal);
//         case KEY_CH:     return cosh (rightVal);
//         case KEY_TH:     return tanh (rightVal);
//         case KEY_CTH:    return 1 / tanh (rightVal);
        
//         case KEY_UKNOWN: 
//             ERROR_LOG ("%s", "Uknown math operation in node"); 
//             return NAN;
        
//         default:
//             assert (0 && "Bro, add another case for NodeCalculateDoMath()");
//     }
// }

// ============= SIMPLIFICATION =============

#define NUM_(num)                                                               \
        NodeCtorAndFill (tree, TYPE_CONST_NUM, {.number = num}, NULL, NULL)

void TreeSimplify (program_t *program, tree_t *tree)
{
    assert (program);
    assert (tree);

    bool modifiedFirst = true;
    bool modifiedSecond = true;
    while (modifiedFirst || modifiedSecond)
    {
        modifiedFirst  = false;
        modifiedSecond = false;

        tree->root = NodeSimplifyCalc (tree, tree->root, &modifiedFirst);
        TREE_DUMP (program, tree, "%s", "After NodeSimplifyCalc()");

        tree->root = NodeSimplifyTrivial (tree, tree->root, &modifiedSecond);
        TREE_DUMP (program, tree, "%s", "After NodeSimplifyTrivial()");
    }
}

node_t *NodeSimplifyCalc (tree_t *tree, node_t *node, bool *modified)
{
    assert (tree);
    assert (node);
    assert (modified);

    if (node->left != NULL)
        node->left = NodeSimplifyCalc (tree, node->left, modified);

    if (node->right != NULL)
        node->right = NodeSimplifyCalc (tree, node->right, modified);
    
    if (node->right == NULL) 
        return node;

    valueNumber_t leftVal  = NAN;
    valueNumber_t rightVal = NAN;

    if (node->left != NULL)
        leftVal = node->left->value.number;
    if (node->right != NULL)
        rightVal = node->right->value.number;

    node_t *newNode = NULL;

    if (node->right->type == TYPE_CONST_NUM && 
        (node->left == NULL || node->left->type  == TYPE_CONST_NUM))
    {
        switch (node->value.idx)
        {
            case KEY_ADD:    newNode = NUM_ (leftVal + rightVal);                             break;
            case KEY_SUB:    newNode = NUM_ (leftVal - rightVal);                             break;
            case KEY_MUL:    newNode = NUM_ (leftVal * rightVal);                             break;
            case KEY_DIV:    newNode = NUM_ (leftVal / rightVal);                             break;
            case KEY_POW:    newNode = NUM_ (valueNumber_t(pow (leftVal, rightVal)));         break;
            case KEY_LOG:    newNode = NUM_ (valueNumber_t(logWithBase (leftVal, rightVal))); break;
            case KEY_LN:     newNode = NUM_ (valueNumber_t(log (rightVal)));                  break;
            case KEY_SIN:    newNode = NUM_ (valueNumber_t(sin (rightVal)));                  break;
            case KEY_COS:    newNode = NUM_ (valueNumber_t(cos (rightVal)));                  break;
            case KEY_TG:     newNode = NUM_ (valueNumber_t(tan (rightVal)));                  break;
            case KEY_CTG:    newNode = NUM_ (valueNumber_t(1 / tan (rightVal)));              break;
            case KEY_ARCSIN: newNode = NUM_ (valueNumber_t(asin (rightVal)));                 break;
            case KEY_ARCCOS: newNode = NUM_ (valueNumber_t(acos (rightVal)));                 break;
            case KEY_ARCTG:  newNode = NUM_ (valueNumber_t(atan (rightVal)));                 break;
            case KEY_ARCCTG: newNode = NUM_ (valueNumber_t(1 / atan (rightVal)));             break;
            case KEY_SH:     newNode = NUM_ (valueNumber_t(sinh (rightVal)));                 break;
            case KEY_CH:     newNode = NUM_ (valueNumber_t(cosh (rightVal)));                 break;
            case KEY_TH:     newNode = NUM_ (valueNumber_t(tanh (rightVal)));                 break;
            case KEY_CTH:    newNode = NUM_ (valueNumber_t(1 / tanh (rightVal)));             break;
            
            case KEY_UKNOWN: 
                ERROR_LOG ("%s", "Uknown math operation in node"); 
                return NULL;
            
            default:
                assert (0 && "Bro, add another case for NodeSimplifyCalc()");
        }
    }
    
    if (newNode == NULL)
        return node;

    TreeDelete (tree, &node);

    *modified = true;

    return newNode;
}

#define MUL_(left, right)                                                        \
        NodeCtorAndFill (tree, TYPE_KEYWORD, {.idx = KEY_MUL},             \
                         left, right)

#define cL NodeCopy (node->left,    tree)
#define cR NodeCopy (node->right,   tree)

#define L left
#define R right

#define IS_VALUE_(childNode, numberValue)                        \
        (node->childNode->type == TYPE_CONST_NUM &&              \
        IsEqual (node->childNode->value.number, numberValue))   

node_t *NodeSimplifyTrivial (tree_t *tree, node_t *node, bool *modified)
{
    assert (tree);
    assert (node);
    assert (modified);

    if (node->left != NULL)
        node->left = NodeSimplifyTrivial (tree, node->left, modified);

    if (node->right != NULL)
        node->right = NodeSimplifyTrivial (tree, node->right, modified);
    
    if (node->right == NULL) 
        return node;

    node_t *newNode = NULL;

    switch (node->value.idx)
    {
        case KEY_ADD:
            if (IS_VALUE_ (L, 0))
                newNode = cR;
            else if (IS_VALUE_ (R, 0))
                newNode = cL;
            break;

        case KEY_SUB:
            if (IS_VALUE_ (L, 0))
                newNode = MUL_ (NUM_(-1), cR);
            else if (IS_VALUE_ (R, 0))
                newNode = cL;
            break;

        case KEY_MUL:
            if (IS_VALUE_ (L, 1))
                newNode = cR;
            else if (IS_VALUE_ (R, 1))
                newNode = cL;
            else if (IS_VALUE_ (L, 0) || IS_VALUE_ (R, 0))
                newNode = NUM_ (0);
            break;
        
        case KEY_DIV:
            if (IS_VALUE_ (R, 1)) // (...) / 1
                newNode = cL;
            else if (IS_VALUE_ (L, 0)) // 0 / (...) 
                newNode = NUM_ (0);
            break;

        case KEY_POW:
            if (IS_VALUE_ (R, 1)) // ^1
                newNode = cL;
            else if (IS_VALUE_ (R, 0) || IS_VALUE_ (L, 1)) // (...)^0 || 1^(...)
                newNode = NUM_ (1);
            break;

        default: break;
    }

    if (newNode == NULL) 
        return node;

    TreeDelete (tree, &node);

    *modified = true;

    return newNode;
}

#undef MUL_
#undef cL
#undef cR
#undef L
#undef R
#undef IS_VALUE_

#undef NUM_
