#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

#include "tree_ast.h"

#include "tree.h"
#include "tree_load_infix.h"
#include "utils.h"
#include "float_math.h"

static double NodeCalculateDoMath       (node_t *node, double leftVal, double rightVal);
static double NodeGetVariable           (program_t *program, node_t *node);
static void AskVariableValue            (program_t *program, node_t *node);

static node_t *NodeSimplifyCalc         (tree_t *tree, node_t *node, bool *modified);
static node_t *NodeSimplifyTrivial      (tree_t *tree, node_t *node, bool *modified);

// NOTE: programNumber - how many times to programerentiate, idk better naming
int ProgramCtor (program_t *program, size_t variablesCapacity)
{
    assert (program);

    TREE_DO_AND_RETURN (LogCtor (&program->log));
    
    program->variablesCapacity = variablesCapacity;
    program->variablesSize     = 0;

    program->variables = (variable_t *) calloc (variablesCapacity, sizeof (variable_t));
    if (program->variables == NULL)
    {
        ERROR_LOG ("Error allocating memory for program->variables - %s",
                    strerror (errno));

        return TREE_ERROR_COMMON |
               COMMON_ERROR_ALLOCATING_MEMORY;
    }
    program->buffer            = NULL;

    TREE_DO_AND_RETURN (TREE_CTOR (&program->expression, &program->log));

    size_t len = 0;
    TREE_DO_AND_RETURN (TreeLoadInfixFromFile (program, &program->expression, 
                        ktreeSaveFileName, &program->buffer, &len));

    return TREE_OK;
}

void ProgramDtor (program_t *program)
{
    assert (program);

    LogDtor (&program->log);

    TreeDtor (&program->expression);

    free (program->variables);
    program->variables = NULL;
    program->variablesCapacity = 0;
    program->variablesSize     = 0;

    free (program->buffer);
    program->buffer = NULL;
}

// int TreeSaveToFile (tree_t *tree, const char *fileName)
// {
//     FILE *outputFile = fopen (fileName, "w");
//     if (outputFile == NULL)
//     {
//         ERROR_LOG ("Error opening file \"%s\"", fileName);
        
//         return TREE_ERROR_COMMON |
//                COMMON_ERROR_OPENING_FILE;
//     }

//     int status = NodeSaveToFile (tree->root, outputFile);

//     fclose (outputFile);

//     return status;
// }

// int NodeSaveToFile (node_t *node, FILE *file)
// {
//     assert (node);
//     assert (file);

//     // NOTE: maybe add macro for printf to check it's return code
//     if (node->type == TYPE_CONST_NUM)
//     {
//         fprintf (file, "(\"%g\"", node->value.number);
//     }
//     else if (node->type == TYPE_MATH_OPERATION)
//     {
//         // FIXME:
//     }
//     else if (node->type == TYPE_VARIABLE)
//     {
//         // FIXME:
//     }

//     if (node->left != NULL)
//         NodeSaveToFile (node->left, file);
//     else
//         fprintf (file, "%s", "nil");

//     if (node->right != NULL)
//         NodeSaveToFile (node->right, file);
//     else
//         fprintf (file, "%s", "nil");

//     fprintf (file, "%s", ")");

//     return TREE_OK;
// }

const char *GetTypeName (type_t type)
{
    switch (type)
    {
        case TYPE_UKNOWN:           return "uknown";
        case TYPE_CONST_NUM:        return "number";
        case TYPE_MATH_OPERATION:   return "operation";
        case TYPE_VARIABLE:         return "variable";

        default:                    return "ERROR";
    }
}

void TryToFindOperator (char *str, int len, type_t *type, treeDataType *value)
{
    assert (str);

    for (size_t i = 0; i < kNumberOfKeywords; i++)
    {
        if (strncmp (str, keywords[i].name, (size_t) len) == 0)
        {
            *type = TYPE_MATH_OPERATION;
            value->idx = keywords[i].idx;

            DEBUG_LOG ("FOUND \"%s\"", keywords[i].name);
        }
    }
}

variable_t *FindVariableByName (program_t *program, char *varName, size_t varNameLen)
{
    assert (program);
    assert (varName);

    DEBUG_LOG ("varName = \"%.*s\"", (int) varNameLen, varName);
    DEBUG_VAR ("%lu", varNameLen);

    for (size_t i = 0; i < program->variablesSize; i++)
    {
        DEBUG_VAR ("%lu", i);
        DEBUG_PTR (program->variables[i].name);
        DEBUG_LOG ("program->variables[i].name = \"%.*s\"", 
                   (int)program->variables[i].len,
                   program->variables[i].name);
        
        int res = 12;
        if ((res = strncmp (program->variables[i].name, varName, varNameLen)) == 0)
        {
            return &program->variables[i];
        }
        DEBUG_VAR ("%d", res);
    }

    return NULL;
}

variable_t *FindVariableByIdx (program_t *program, size_t idx)
{
    assert (program);

    for (size_t i = 0; i < program->variablesSize; i++)
    {
        if (program->variables[i].idx == idx)
            return &program->variables[i];
    }

    return NULL;
}

const keyword_t *FindKeywordByIdx (keywordIdxes_t idx)
{
    for (size_t i = 0; i < kNumberOfKeywords; i++)
    {
        if (keywords[i].idx == idx)
            return &keywords[i];
    }

    return NULL;
}

int FindOrAddVariable (program_t *program, char **curPos, 
                       size_t len, type_t *type, treeDataType *value)
{
    assert (program);
    assert (curPos);
    assert (*curPos);
    assert (type);
    assert (value);

    *type = TYPE_VARIABLE;
    char *varName = *curPos;
    // (*curPos)[len] = '\0'; // TODO: this can crash something

    variable_t *variable = FindVariableByName (program, varName, len);

    if (variable == NULL)
    {
        // NOTE: New function - add new variable ?
        TREE_DO_AND_RETURN (CheckForReallocVariables (program));

        size_t idx = (program->variablesSize);
        value->idx = idx;

        program->variables[idx].name = varName;
        program->variables[idx].len = len;
        program->variables[idx].idx = idx;
        program->variables[idx].value = NAN;

        DEBUG_LOG ("%.*s", (int)program->variables[idx].len, program->variables[idx].name);

        program->variablesSize++;
    }
    else
    {
        value->idx = variable->idx;
    }

    DEBUG_VAR ("%lu", program->variablesSize);
    DEBUG_LOG ("variable name is '%.*s'", 
               (int)program->variables[program->variablesSize].len, varName);
    DEBUG_LOG ("(*value).idx = '%lu'", (*value).idx);

    return TREE_OK;
}


int CheckForReallocVariables (program_t *program)
{
    assert (program);

    if (program->variablesSize >= program->variablesCapacity)
    {
        if (program->variablesCapacity == 0) 
            program->variablesCapacity = 1;

        program->variablesCapacity *= 2;

        variable_t *newVariables = (variable_t *) realloc (program->variables, 
                                                           program->variablesCapacity * sizeof (variable_t));
        if (newVariables == NULL)
        {
            ERROR_LOG ("Error reallocating memory for program->variables - %s", strerror (errno));

            return TREE_ERROR_COMMON |
                   COMMON_ERROR_ALLOCATING_MEMORY;
        }

        program->variables = newVariables;

    }

    return TREE_OK;
}

// =============  CALCULATION   =============

int TreeCalculate (program_t *program, tree_t *expression)
{
    assert (program);
    assert (expression);

    DEBUG_VAR ("%lu", program->variablesSize);
    
    double result = NodeCalculate (program, expression->root);

    PRINT ("Expression is equals to %g\n", result);

    return TREE_OK;
}

double NodeCalculate (program_t *program, node_t *node)
{
    assert (program);
    assert (node);

    double leftVal  = NAN;
    double rightVal = NAN;

    if (node->left != NULL)
        leftVal = NodeCalculate (program, node->left);

    if (node->right != NULL)
        rightVal = NodeCalculate (program, node->right);

    switch (node->type)
    {
        case TYPE_UKNOWN:
            ERROR_LOG ("%s", "Uknown node while calculating tree");

            return TREE_ERROR_INVALID_NODE;
            
        case TYPE_CONST_NUM:
            return node->value.number;

        case TYPE_MATH_OPERATION:
            return NodeCalculateDoMath (node, leftVal, rightVal);

        case TYPE_VARIABLE:
            return NodeGetVariable (program, node);
    
        default:
            assert (0 && "Add new case in NodeCalctulate or wtf bro");
    }
}

double NodeCalculateDoMath (node_t *node, double leftVal, double rightVal)
{
    assert (node);

    switch (node->value.idx)
    {
        case OP_ADD:    return leftVal + rightVal;
        case OP_SUB:    return leftVal - rightVal;
        case OP_MUL:    return leftVal * rightVal;
        case OP_DIV:    return leftVal / rightVal;
        case OP_POW:    return pow (leftVal, rightVal);
        case OP_LOG:    return logWithBase (leftVal, rightVal);
        case OP_LN:     return log (rightVal);
        case OP_SIN:    return sin (rightVal);
        case OP_COS:    return cos (rightVal);
        case OP_TG:     return tan (rightVal);
        case OP_CTG:    return 1 / tan (rightVal);
        case OP_ARCSIN: return asin (rightVal);
        case OP_ARCCOS: return acos (rightVal);
        case OP_ARCTG:  return atan (rightVal);
        case OP_ARCCTG: return 1 / atan (rightVal);
        case OP_SH:     return sinh (rightVal);
        case OP_CH:     return cosh (rightVal);
        case OP_TH:     return tanh (rightVal);
        case OP_CTH:    return 1 / tanh (rightVal);
        
        case OP_UKNOWN: 
            ERROR_LOG ("%s", "Uknown math operation in node"); 
            return NAN;
        
        default:
            assert (0 && "Bro, add another case for NodeCalculateDoMath()");
    }
}

double NodeGetVariable (program_t *program, node_t *node)
{
    assert (program);
    assert (node);

    // DEBUG_VAR ("%lu", node->value.idx);

    if (isnan (program->variables[node->value.idx].value))
    {
        AskVariableValue (program, node);
    }
    
    return program->variables[node->value.idx].value;
}

void AskVariableValue (program_t *program, node_t *node)
{
    assert (program);
    assert (node);

    size_t idx = node->value.idx;

    DEBUG_VAR ("%lu", idx);

    int status   = 0;
    int attempts = 0;

    while (attempts < 5 && status != 1)
    {
        if (attempts >= 1)
        {
            ERROR_PRINT ("%s", "This is not a float point number. Please, try again");
        }

        PRINT ("Input value of variable '%.*s'\n"
               " > ",
               (int)program->variables[idx].len,
               program->variables[idx].name);

        status = scanf ("%lg", &program->variables[idx].value);
        ClearBuffer ();

        attempts++;
    }

    if (attempts >= 5)
    {
        program->variables[idx].value = 0;
        PRINT ("I am tired.\n"
               "Value of '%.*s' was set to %g.\n"
               "Think about your behavior", 
               (int) program->variables[idx].len, program->variables[idx].name, 
               program->variables[idx].value);
    }
}

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

    double leftVal  = NAN;
    double rightVal = NAN;

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
            case OP_ADD:    newNode = NUM_ (leftVal + rightVal);                    break;
            case OP_SUB:    newNode = NUM_ (leftVal - rightVal);                    break;
            case OP_MUL:    newNode = NUM_ (leftVal * rightVal);                    break;
            case OP_DIV:    newNode = NUM_ (leftVal / rightVal);                    break;
            case OP_POW:    newNode = NUM_ (pow (leftVal, rightVal));               break;
            case OP_LOG:    newNode = NUM_ (logWithBase (leftVal, rightVal));       break;
            case OP_LN:     newNode = NUM_ (log (rightVal));                        break;
            case OP_SIN:    newNode = NUM_ (sin (rightVal));                        break;
            case OP_COS:    newNode = NUM_ (cos (rightVal));                        break;
            case OP_TG:     newNode = NUM_ (tan (rightVal));                        break;
            case OP_CTG:    newNode = NUM_ (1 / tan (rightVal));                    break;
            case OP_ARCSIN: newNode = NUM_ (asin (rightVal));                       break;
            case OP_ARCCOS: newNode = NUM_ (acos (rightVal));                       break;
            case OP_ARCTG:  newNode = NUM_ (atan (rightVal));                       break;
            case OP_ARCCTG: newNode = NUM_ (1 / atan (rightVal));                   break;
            case OP_SH:     newNode = NUM_ (sinh (rightVal));                       break;
            case OP_CH:     newNode = NUM_ (cosh (rightVal));                       break;
            case OP_TH:     newNode = NUM_ (tanh (rightVal));                       break;
            case OP_CTH:    newNode = NUM_ (1 / tanh (rightVal));                   break;
            
            case OP_UKNOWN: 
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
        NodeCtorAndFill (tree, TYPE_MATH_OPERATION, {.idx = OP_MUL},             \
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
        case OP_ADD:
            if (IS_VALUE_ (L, 0))
                newNode = cR;
            else if (IS_VALUE_ (R, 0))
                newNode = cL;
            break;

        case OP_SUB:
            if (IS_VALUE_ (L, 0))
                newNode = MUL_ (NUM_(-1), cR);
            else if (IS_VALUE_ (R, 0))
                newNode = cL;
            break;

        case OP_MUL:
            if (IS_VALUE_ (L, 1))
                newNode = cR;
            else if (IS_VALUE_ (R, 1))
                newNode = cL;
            else if (IS_VALUE_ (L, 0) || IS_VALUE_ (R, 0))
                newNode = NUM_ (0);
            break;
        
        case OP_DIV:
            if (IS_VALUE_ (R, 1)) // (...) / 1
                newNode = cL;
            else if (IS_VALUE_ (L, 0)) // 0 / (...) 
                newNode = NUM_ (0);
            break;

        case OP_POW:
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