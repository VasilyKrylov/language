#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>

#include "tokenizator.h"

#include "utils.h"

int FillTokensArray (char *buffer, tokensArray_t *tokens, variablesTable_t *namesTable);

int TokenAddNumber (char **curPos, tokensArray_t *tokens, size_t line, size_t position);


int TokenAddVariable (char **curPos, tokensArray_t *tokens, variablesTable_t *namesTable, 
                    size_t line, size_t position);

int TokenAdd (tokensArray_t *tokens, type_t type, value_t value, 
              size_t line, size_t position);


int CheckForReallocTokens (tokensArray_t *tokens);

int CheckForReallocVariables (variablesTable_t *namesTable);


int FindOrAddVariable (variablesTable_t *variables, char *varName, size_t len, 
                       size_t *idx);


int TokensArrayCtor (tokensArray_t *tokens)
{
    assert (tokens);
    
    const size_t kTokensInitCapacity = 16;

    tokens->size     = 0;
    tokens->capacity = kTokensInitCapacity;

    tokens->data = (token_t *) calloc (tokens->capacity, sizeof (token_t));

    if (tokens->data == NULL)
    {
        ERROR_LOG ("Error allocating memory for tokens->data - %s",
                    strerror (errno));

        return TREE_ERROR_COMMON |
               COMMON_ERROR_ALLOCATING_MEMORY;
    }

    return TREE_OK;
}

void TokensArrayDtor (tokensArray_t *tokens)
{
    assert (tokens);
    
    free (tokens->data);
    tokens->data = NULL;

    tokens->size     = 0;
    tokens->capacity = 0;
}

int VariablesTableCtor (variablesTable_t *variablesTable)
{
    assert (variablesTable);

    const size_t kNamesTableInitCapacity = 16;

    variablesTable->size     = 0;
    variablesTable->capacity = kNamesTableInitCapacity;

    variablesTable->data = (variable_t *) calloc (variablesTable->capacity, sizeof (variable_t));

    if (variablesTable->data == NULL)
    {
        ERROR_LOG ("Error allocating memory for namesTable-data - %s",
                    strerror (errno));

        return TREE_ERROR_COMMON |
               COMMON_ERROR_ALLOCATING_MEMORY;
    }

    return TREE_OK;
}

void VariablesTableDtor (variablesTable_t *varaiblesTable)
{
    assert (varaiblesTable);

    free (varaiblesTable->data);
    varaiblesTable->data = NULL;

    varaiblesTable->size     = 0;
    varaiblesTable->capacity = 0;
}

int GetTokens (const char *fileName, program_t *program)
{
    assert (program);

    size_t bufferLen = 0;
    program->buffer = ReadFile (fileName, &bufferLen);

    if (program->buffer == NULL)
        return TREE_ERROR_COMMON |
               COMMON_ERROR_READING_FILE;

    TREE_DO_AND_RETURN (FillTokensArray (program->buffer, &program->tokens, &program->variables));

    DEBUG_LOG ("Tokens number - %lu", program->tokens.size);

    return TREE_OK;
}

int FillTokensArray (char *buffer, tokensArray_t *tokens, variablesTable_t *variablesTable)
{
    assert (buffer);
    assert (tokens);

    size_t line     = 0;
    size_t position = 0;

    for (char *curPos = buffer; *curPos != '\0';)
    {
        curPos = SkipSpacesAndCount (curPos, &line, &position);

        DEBUG_STR (curPos);

        // FIXME: calc position after adding correct token

        if (isdigit (*curPos))
        {
            TREE_DO_AND_RETURN (TokenAddNumber (&curPos, tokens, line, position));

            DEBUG_LOG ("After adding number: \"%s\"", curPos);
            
            continue;
        }

        bool found = false;

        for (size_t i = 0; i < kNumberOfKeywords; i++)
        {
            if (strncmp (curPos, kKeywords[i].name, kKeywords[i].nameLen) == 0)
            {
                TREE_DO_AND_RETURN (TokenAdd (tokens, TYPE_KEYWORD, {.idx = kKeywords[i].idx}, 
                                    line, position));
                found = true;

                curPos += kKeywords[i].nameLen;

                DEBUG_STR (kKeywords[i].name);
                DEBUG_VAR ("%lu", line);
                
                break;
            }
        }
        if (found) 
            continue;

        TREE_DO_AND_RETURN (TokenAddVariable (&curPos, tokens, variablesTable, line, position));
    }

    return TREE_OK;
}

int CheckForReallocTokens (tokensArray_t *tokens)
{
    assert (tokens);

    if (tokens->size < tokens->capacity)
        return TREE_OK;

    if (tokens->capacity == 0)
        tokens->capacity = 1;
    
    tokens->capacity *= 2;

    token_t *newData = (token_t *) realloc (tokens->data, tokens->capacity * sizeof (token_t));

    if (newData == NULL)
    {
        ERROR_LOG ("Error reallocating memory - %s", strerror (errno));

        return TREE_ERROR_COMMON |
                COMMON_ERROR_ALLOCATING_MEMORY;
    }
    
    tokens->data = newData;

    return TREE_OK;
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

int TokenAdd (tokensArray_t *tokens, type_t type, value_t value, 
              size_t line, size_t position)
{
    assert (tokens);

    TREE_DO_AND_RETURN (CheckForReallocTokens (tokens));

    tokens->data[tokens->size] = {.type = type, .value = value,
                                  .line = line, .position = position};
    tokens->size++;

    return TREE_OK;
}

int TokenAddNumber (char **curPos, tokensArray_t *tokens, size_t line, size_t position)
{
    assert (curPos);
    assert (*curPos);
    assert (tokens);

    int bytesRead = 0;
    int number = 0;

    // FIXME: this can read negative numbers, which isn't in our Gramma
    int status = sscanf (*curPos, "%d%n", &number, &bytesRead);
    if (status < 1)
        return TREE_ERROR_COMMON |
               COMMON_ERROR_SSCANF;

    *curPos += bytesRead;

    return TokenAdd (tokens, TYPE_CONST_NUM, {.number = number}, line, position);
}

int TokenAddVariable (char **curPos, tokensArray_t *tokens, variablesTable_t *variables, 
                      size_t line, size_t position)
{
    assert (curPos);
    assert (*curPos);
    assert (tokens);
    assert (variables);
    
    if (!isalpha (**curPos) && **curPos != '_')
    {
        ERROR_LOG ("line %lu, potisition %lu - Uknown token: Variables can't start with '%c'", 
                   line, position, **curPos);

        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    }

    char *varName = *curPos;
        
    (*curPos)++;

    DEBUG_VAR ("%p", *curPos);

    while (isalpha (**curPos) || isdigit (**curPos) || **curPos == '_')
    {
        DEBUG_LOG ("**curPos(variable name) = \"%c\"", **curPos);
        DEBUG_VAR ("%p", *curPos);
        
        (*curPos)++;
    }

    size_t idx = 0;

    TREE_DO_AND_RETURN (FindOrAddVariable (variables, varName, size_t(*curPos - varName), &idx));

    TREE_DO_AND_RETURN (TokenAdd (tokens, TYPE_VARIABLE, {.idx = idx}, line, position));
    
    return TREE_OK;
}

int FindOrAddVariable (variablesTable_t *variables, char *varName, size_t len, 
                       size_t *idx)
{
    assert (variables);
    assert (varName);
    assert (idx);

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

    return TREE_OK;
}
