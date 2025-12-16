#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>

#include "tokenizator.h"

#include "utils.h"

int FillTokensArray (char *buffer, tokensArray_t *tokens, variablesTable_t *variables);

int TokenAddNumber (char **curPos, tokensArray_t *tokens, size_t line, size_t position);


int TokenAddVariable (char **curPos, tokensArray_t *tokens, variablesTable_t *variables, 
                    size_t line, size_t position);

int TokenAdd (tokensArray_t *tokens, type_t type, value_t value, 
              size_t line, size_t position);


int CheckForReallocTokens (tokensArray_t *tokens);


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

int VariablesTableCtor (variablesTable_t *variables)
{
    assert (variables);

    const size_t kNamesTableInitCapacity = 16;

    variables->size     = 0;
    variables->capacity = kNamesTableInitCapacity;

    variables->data = (variable_t *) calloc (variables->capacity, sizeof (variable_t));

    if (variables->data == NULL)
    {
        ERROR_LOG ("Error allocating memory for variables->data - %s",
                    strerror (errno));

        return TREE_ERROR_COMMON |
               COMMON_ERROR_ALLOCATING_MEMORY;
    }

    return TREE_OK;
}

void VariablesTableDtor (variablesTable_t *variables)
{
    assert (variables);

    free (variables->data);
    variables->data = NULL;

    variables->size     = 0;
    variables->capacity = 0;
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

int FillTokensArray (char *buffer, tokensArray_t *tokens, variablesTable_t *variables)
{
    assert (buffer);
    assert (tokens);

    size_t line     = 1;
    size_t position = 1;

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

        // FIXME: TryToFindOperator()
        for (size_t i = 0; i < kNumberOfKeywords; i++)
        {
            if (strncmp (curPos, kKeywords[i].name, kKeywords[i].nameLen) == 0)
            {
                TREE_DO_AND_RETURN (TokenAdd (tokens, TYPE_KEYWORD, {.idx = kKeywords[i].idx}, 
                                    line, position));
                found = true;

                curPos += kKeywords[i].nameLen;

                DEBUG_STR (kKeywords[i].name);
                DEBUG_VAR ("%d", kKeywords[i].idx);
                DEBUG_VAR ("%lu", line);
                
                break;
            }
        }

        if (found) continue;

        TREE_DO_AND_RETURN (TokenAddVariable (&curPos, tokens, variables, line, position));
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

    DumpVariables (variables);

    return TREE_OK;
}

void DumpTokens (program_t *program)
{
    assert (program);

    DEBUG_PRINT ("%s", "\n========== TOKENS ==========\n");

    for (size_t i = 0; i < program->tokens.size; i++)
    {
        token_t *token = &program->tokens.data[i];

        DEBUG_PRINT ("token[%lu]: \n", i);

        DEBUG_PRINT ("%s", "\t ");
        PrintToken (stderr, program, token);
        DEBUG_PRINT ("%s", "\n");

        DEBUG_PRINT ("\t type = %s\n", GetTypeName (token->type));

        if (token->type == TYPE_CONST_NUM)
            DEBUG_PRINT ("\t value.number = " VALUE_NUMBER_FSTRING, token->value.number);
        else
            DEBUG_PRINT ("\t value.idx = %lu", token->value.idx);

        DEBUG_PRINT ("%s", "\n");

        DEBUG_PRINT ("\t line = %lu\n", token->line);
        DEBUG_PRINT ("\t position = %lu\n", token->line);
    }
}

void DumpVariables (variablesTable_t *variables)
{
    assert (variables);

    DEBUG_PRINT ("%s", "\n========== Variables ==========\n");

    for (size_t i = 0; i < variables->size; i++)
    {
        variable_t *var = &variables->data[i];

        DEBUG_LOG ("variables[%lu]:", i);
        DEBUG_LOG ("\t idx = %lu", var->idx);
        DEBUG_LOG ("\t name = \"%s\"", var->name);
        DEBUG_LOG ("\t len = %lu", var->len);
    }
}


int PrintToken (FILE *file, program_t *program, token_t *token)
{
    assert (file);
    assert (token);

    switch (token->type)
    {
        case TYPE_UKNOWN:           fprintf (file, "UKNOWN");                                   break;
        case TYPE_CONST_NUM:        fprintf (file, VALUE_NUMBER_FSTRING, token->value.number);  break;

        case TYPE_KEYWORD:          
        {
            const keyword_t *keyword = FindKeywordByIdx ((keywordIdxes_t) token->value.idx);
            
            if (keyword == NULL)
                fprintf (file, "NULL keyword");
            else
                fprintf (file, "%s", keyword->standardName);                  
            break;
        }
        case TYPE_VARIABLE:         
        {
            const variable_t *var = FindVariableByIdx (&program->variables, (size_t) token->value.idx);

            if (var == NULL)
                fprintf (file, "NULL variable");
            else
                fprintf (file, "%.*s", (int)var->len, var->name);
            break;
        }

        default:
            fprintf (file, "error");

            return TREE_ERROR_INVALID_NODE;
    }

    return TREE_OK;
}