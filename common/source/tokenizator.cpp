#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>

#include "tokenizator.h"

#include "utils.h"

int FillTokensArray (char *buffer, program_t *program);

int TryToFindOperator (char **curPos, program_t *program, size_t line, size_t position);

int TokenAddNumber (char **curPos, tokensArray_t *tokens, size_t line, size_t position);

int TokenAddName (char **curPos, tokensArray_t *tokens, namesTable_t *namesTable,
                  size_t line, size_t position);

int TokenAdd (tokensArray_t *tokens, type_t type, value_t value, 
              size_t line, size_t position);

int CheckForReallocTokens (tokensArray_t *tokens);

#define LEXICAL_ERROR(format, ...)                                      \
        {                                                               \
            ERROR_PRINT ("%s:%lu:%lu Lexical Error - " format,          \
                         tokens->fileName,                              \
                         line,                                          \
                         position,                                      \
                         __VA_ARGS__);                                  \
                                                                        \
            return TREE_ERROR_SYNTAX_IN_SAVE_FILE;                      \
        }

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

    tokens->fileName = NULL;

    tokens->size     = 0;
    tokens->capacity = 0;
}

int GetTokens (const char *fileName, program_t *program)
{
    assert (program);

    size_t bufferLen = 0;
    program->buffer = ReadFile (fileName, &bufferLen);

    if (program->buffer == NULL)
        return TREE_ERROR_COMMON |
               COMMON_ERROR_READING_FILE;

    program->tokens.fileName = fileName;

    TREE_DO_AND_CLEAR (FillTokensArray (program->buffer, program), 
                       DumpTokens (program));

    DEBUG_LOG ("Tokens number - %lu", program->tokens.size);

    return TREE_OK;
}

#define IS_PREVIOUS_TOKEN_KEYWORD(keyword)                                      \
        (program->tokens.size >= 1 &&                                           \
         program->tokens.data[program->tokens.size - 1].type == TYPE_KEYWORD && \
         program->tokens.data[program->tokens.size - 1].value.idx == keyword)

int FillTokensArray (char *buffer, program_t *program)
{
    assert (buffer);
    assert (program);

    size_t line     = 1;
    size_t position = 1;

    for (char *curPos = buffer; *curPos != '\0';)
    {
        curPos = SkipSpacesAndCount (curPos, &line, &position);

        DEBUG_STR (curPos);
        DEBUG_VAR ("%lu", line);
        DEBUG_VAR ("%lu", position);

        char *curPosBeforeToken = curPos;

        // FIXME: calc position after adding correct token

        if (isdigit (*curPos))
        {
            TREE_DO_AND_RETURN (TokenAddNumber (&curPos, &program->tokens, line, position));

            DEBUG_LOG ("After adding number: \"%s\"", curPos);

            position += size_t (curPos - curPosBeforeToken);
            
            continue;
        }

        int status = TryToFindOperator (&curPos, program, line, position);
        if (status == TREE_OK)
        {
            position += size_t (curPos - curPosBeforeToken) / 2; 
            // 2 bytes per char moment. I do not want to use windows CP-1251

            continue;
        }

        TREE_DO_AND_RETURN (TokenAddName (&curPos, &program->tokens, &program->namesTable, 
                                          line, position));

        position += size_t (curPos - curPosBeforeToken);
    }

    return TREE_OK;
}

#undef IS_PREVIOUS_TOKEN_KEYWORD

int TryToFindOperator (char **curPos, program_t *program, size_t line, size_t position)
{
    assert (curPos);
    assert (*curPos);
    assert (program);

    for (size_t i = 0; i < kNumberOfKeywords; i++)
    {
        if (strncmp (*curPos, kKeywords[i].name, kKeywords[i].nameLen) == 0)
        {
            TREE_DO_AND_RETURN (TokenAdd (&program->tokens, TYPE_KEYWORD, {.idx = kKeywords[i].idx}, 
                                          line, position));

            (*curPos) += kKeywords[i].nameLen;

            DEBUG_STR (kKeywords[i].name);
            DEBUG_VAR ("%d", kKeywords[i].idx);
            DEBUG_VAR ("%lu", line);
            
            return TREE_OK;
        }
    }

    return TREE_ERROR_INVALID_TOKEN;
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

int TokenAddName (char **curPos, tokensArray_t *tokens, namesTable_t *namesTable,
                  size_t line, size_t position)
{
    assert (curPos);
    assert (*curPos);
    assert (tokens);
    assert (namesTable);
    
    if (!isalpha (**curPos) && **curPos != '_')
    {
        LEXICAL_ERROR ("Uknown token: Variables can't start with '%c'", 
                       **curPos);
    }

    char *nameStr = *curPos;
        
    (*curPos)++;

    DEBUG_VAR ("%p", *curPos);

    while (isalpha (**curPos) || isdigit (**curPos) || **curPos == '_')
    {
        DEBUG_LOG ("**curPos(variable name) = \"%c\"", **curPos);
        DEBUG_VAR ("%p", *curPos);
        
        (*curPos)++;
    }

    size_t idx = 0;

    TREE_DO_AND_RETURN (NamesTableFindOrAdd (namesTable, nameStr, size_t(*curPos - nameStr), &idx));

    TREE_DO_AND_RETURN (TokenAdd (tokens, TYPE_NAME, {.idx = idx}, line, position));

    NamesTableDump (namesTable);

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
        ON_DEBUG (
            PrintToken (stderr, program, token);
        );
        DEBUG_PRINT ("%s", "\n");

        DEBUG_PRINT ("\t type = %s\n", GetTypeName (token->type));

        if (token->type == TYPE_CONST_NUM)
        {
            DEBUG_PRINT ("\t value.number = " VALUE_NUMBER_FSTRING, token->value.number);
        }
        else
        {
            DEBUG_PRINT ("\t value.idx = %lu", token->value.idx);
        }

        DEBUG_PRINT ("%s", "\n");

        DEBUG_PRINT ("\t line = %lu\n", token->line);
        DEBUG_PRINT ("\t position = %lu\n", token->line);
    }
}

void NamesTableDump (namesTable_t *namesTable)
{
    assert (namesTable);

    DEBUG_PRINT ("%s", "\n========== Names Table start ==========\n");

    DEBUG_PRINT ("namesTable->size = %lu", namesTable->size);

    for (size_t i = 0; i < namesTable->size; i++)
    {
        ON_DEBUG (name_t *name = &namesTable->data[i];)

        DEBUG_PRINT ("namesTable[%lu]:\n", i);
        DEBUG_PRINT ("\t idx = %lu\n", name->idx);
        DEBUG_PRINT ("\t name = \"%.*s\"\n", (int) name->len, name->name);
        DEBUG_PRINT ("\t len = %lu\n", name->len);
    }

    DEBUG_PRINT ("%s", "\n========== Names Table end ==========\n");

}

// FIXME: copy-paste. New function - print value based on type
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
        case TYPE_NAME:         
        case TYPE_VARIABLE:         
        {
            const name_t *var = NamesTableFindByIdx (&program->namesTable, (size_t) token->value.idx);

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