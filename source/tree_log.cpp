#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>

#include "tree_log.h"

#include "tree.h"
#include "tree_ast.h"
#include "utils.h"

static size_t imageCounter = 0;

const char * const kBlack       = "#000000";
const char * const kGray        = "#ebebe0";

const char * const kRed         = "#ff0000";
const char * const kViolet      = "#cc99ff";
const char * const kBlue        = "#66ccff";

const char * const kGreen       = "#99ff66";
const char * const kDarkGreen   = "#33cc33";
const char * const kYellow      = "#ffcc00";

const char * const kHeadColor   = kViolet;
const char * const kFreeColor   = kDarkGreen;
const char * const kEdgeNormal  = kBlack;

void TreePrefixPass     (program_t *program, node_t *node, FILE *graphFile);
int TreeDumpImg         (program_t *program, node_t *node);
int DumpMakeConfig      (program_t *program, node_t *node);
int DumpMakeImg         (node_t *node, treeLog_t *log);

int LogCtor (treeLog_t *log)
{
    time_t t = time (NULL);
    struct tm tm = *localtime (&t);

    snprintf (log->logFolderPath, kFileNameLen, "%s%d-%02d-%02d_%02d:%02d:%02d/",
              kParentDumpFolderName,
              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
              tm.tm_hour,        tm.tm_min,     tm.tm_sec);

    snprintf (log->htmlFilePath,        kFileNameLen, "%s%s",
              log->logFolderPath,       kHtmlFileName);

    snprintf (log->imgFolderPath,       kFileNameLen, "%s%s",
              log->logFolderPath,       kImgFolderName);

    snprintf (log->dotFolderPath,       kFileNameLen, "%s%s",
              log->logFolderPath,       kDotFolderName);

    if (SafeMkdir (kParentDumpFolderName) != TREE_OK)
        return TREE_ERROR_COMMON | 
               COMMON_ERROR_CREATING_FILE;

    if (SafeMkdir (log->logFolderPath) != TREE_OK)
        return TREE_ERROR_COMMON | 
               COMMON_ERROR_CREATING_FILE;

    if (SafeMkdir (log->imgFolderPath) != TREE_OK)
        return TREE_ERROR_COMMON | 
               COMMON_ERROR_CREATING_FILE;

    if (SafeMkdir (log->dotFolderPath) != TREE_OK)
        return TREE_ERROR_COMMON | 
               COMMON_ERROR_CREATING_FILE;

    log->htmlFile = fopen (log->htmlFilePath, "w");
    if (log->htmlFile == NULL)
    {
        ERROR_LOG ("Error opening file \"%s\"", log->htmlFilePath);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_OPENING_FILE;
    }
    fprintf (log->htmlFile, "%s", "<pre>\n");

    return TREE_OK;
}

void LogDtor (treeLog_t *log)
{
    fprintf (log->htmlFile, "%s", "</pre>\n");

    fclose (log->htmlFile);
}

int NodeDump (program_t *program, node_t *node,
              const char *file, int line, const char *func, 
              const char *format, ...)
{
    assert (program);
    assert (node);
    assert (file);
    assert (func);
    assert (format);

    DEBUG_PRINT ("%s", "\n========== NODE DUMP START ==========\n");

    treeLog_t *log = &program->log;

    fprintf (log->htmlFile,
             "<h3>NODE DUMP called at %s:%d:%s(): <font style=\"color: green;\">",
             file, line, func);

    va_list  args = {};
    va_start (args, func);
    vfprintf (log->htmlFile, format, args);
    va_end   (args);
    
    fprintf (log->htmlFile,
             "%s",
             "</font></h3>\n");
    
    TREE_DO_AND_RETURN (TreeDumpImg (program, node));

    fprintf (log->htmlFile, "%s", "<hr>\n\n");

    fflush (log->htmlFile);

    DEBUG_PRINT ("%s", "========== NODE DUMP END ==========\n\n");

    return TREE_OK;
}

int TreeDump (program_t *program, tree_t *tree, 
              const char *file, int line, const char *func, 
              const char *format, ...)
{
    assert (tree);
    assert (tree->root);
    assert (format);
    assert (file);
    assert (func);

    DEBUG_PRINT ("%s", "\n========== START OF TREE DUMP TO HTML  ==========\n");

    treeLog_t *log = &program->log;
    
    fprintf (log->htmlFile,
             "<h3>TREE DUMP called at %s:%d:%s(): <font style=\"color: green;\">",
             file, line, func);
        
    va_list args = {};
    va_start (args, func);
    vfprintf (log->htmlFile, format, args);
    va_end   (args);

    fprintf (log->htmlFile, "%s", "</font></h3>\n");
        
    ON_DEBUG (
        fprintf (log->htmlFile,
                 "%s[%p] initialized in {%s:%d}\n",
                 tree->varInfo.name, tree, tree->varInfo.file, tree->varInfo.line);
    );
    fprintf (log->htmlFile,
             "tree->size = %lu;\n",
             tree->size);

    TREE_DO_AND_RETURN (TreeDumpImg (program, tree->root));

    fprintf (log->htmlFile, "%s", "<hr>\n\n");

    fflush (log->htmlFile);

    DEBUG_PRINT ("%s", "========== END OF TREE DUMP TO HTML  ==========\n\n");
    
    return TREE_OK;
}

int TreeDumpImg (program_t *program, node_t *node)
{
    assert (program);
    assert (node);

    TREE_DO_AND_RETURN (DumpMakeConfig (program, node));

    TREE_DO_AND_RETURN (DumpMakeImg (node, &program->log));

    return TREE_OK;
}

// NOTE: better to make argument for dump to choose in wich style to make image (?)
// void TreePrefixPass (program_t *program, node_t *node, FILE *graphFile)
// {
//     assert (program);
//     assert (node);
//     assert (graphFile);

//     fprintf (graphFile,
//             "\tnode%p [shape=Mrecord; style=\"filled\"; fillcolor=\"%s\"; "
//             "label = \"{ type = %s",
//             node, kBlue, GetTypeName(node->type));
    
//     fprintf (graphFile, "%s", " | value = ");

//     int idx = node->value.idx;

//     switch (node->type)
//     {
//         case TYPE_UKNOWN:           fprintf (graphFile, "%g", node->value.number);                      break;
//         case TYPE_CONST_NUM:        fprintf (graphFile, "%g", node->value.number);                      break;
//         case TYPE_MATH_OPERATION:   fprintf (graphFile, "%s", keywords[idx].name);                     break;
        // case TYPE_VARIABLE:         fprintf (graphFile, "%.*s", 
        //                                      (int) program->variables[idx].len, 
        //                                      program->variables[idx].name);                    break;
//         case TYPE_VARIABLE:         fprintf (graphFile, "%d (\\\" \\\")", idx);   break;
//         default:                    fprintf (graphFile, "error");                                       break;
//     }

//     fprintf (graphFile, " | ptr = [%p] | left = [%p] | right = [%p] }\"];\n", 
//                         node, node->left, node->right);

//     DEBUG_VAR ("%p", node);
//     DEBUG_LOG ("\t node->left: %p", node->left);
//     DEBUG_LOG ("\t node->right: %p", node->right);

//     if (node->left != NULL)
//     {
//         DEBUG_LOG ("\t %p->%p\n", node, node->left);

//         fprintf (graphFile, "\tnode%p->node%p\n", node, node->left);
        
//         TreePrefixPass (program, node->left, graphFile);
//     }    

//     if (node->right != NULL)
//     {
//         DEBUG_LOG ("\t %p->%p\n", node, node->left);

//         fprintf (graphFile, "\tnode%p->node%p\n", node, node->right);
        
//         TreePrefixPass (program, node->right, graphFile);
//     }
// }

void TreePrefixPass (program_t *program, node_t *node, FILE *graphFile)
{
    assert (program);
    assert (node);
    assert (graphFile);

    fprintf (graphFile,
             "\tnode%p [shape=Mrecord; style=\"filled\"; fillcolor=",
             node);

    DEBUG_VAR ("%lu", node->value.idx);

    const keyword_t *keyword = FindKeywordByIdx ((keywordIdxes_t) node->value.idx);

    switch (node->type)
    {
        case TYPE_UKNOWN:           fprintf (graphFile, "\"%s\";", kGray);      break;
        case TYPE_CONST_NUM:        fprintf (graphFile, "\"%s\";", kBlue);      break;
        case TYPE_MATH_OPERATION:   if (keyword->isFunction)
                                        fprintf (graphFile, "\"%s\";", kYellow);
                                    else
                                        fprintf (graphFile, "\"%s\"", kGreen);  
                                    break;
        case TYPE_VARIABLE:         fprintf (graphFile, "\"%s\";", kViolet);    break;
        default:                    fprintf (graphFile, "\"%s\";", kRed);      break;
    }

    variable_t *var = FindVariableByIdx (program, (size_t) node->value.idx);

    fprintf (graphFile, " label = \" {");

    switch (node->type)
    {
        case TYPE_UKNOWN:           fprintf (graphFile, "%g", node->value.number);          break;
        case TYPE_CONST_NUM:        fprintf (graphFile, "%g", node->value.number);          break;
        case TYPE_MATH_OPERATION:   fprintf (graphFile, "%s", keyword->name);               break;
        case TYPE_VARIABLE:         fprintf (graphFile, "%.*s", (int)var->len, var->name);  break;
        default:                    fprintf (graphFile, "error");                           break;
    }

    fprintf (graphFile, " }\"];\n");

    DEBUG_PTR (node);
    // DEBUG_LOG ("\t node->left: %p", node->left);
    // DEBUG_LOG ("\t node->right: %p", node->right);

    if (node->left != NULL)
    {
        DEBUG_LOG ("\t %p->%p\n", node, node->left);

        fprintf (graphFile, "\tnode%p->node%p\n", node, node->left);
        
        TreePrefixPass (program, node->left, graphFile);
    }    

    if (node->right != NULL)
    {
        DEBUG_LOG ("\t %p->%p\n", node, node->left);

        fprintf (graphFile, "\tnode%p->node%p\n", node, node->right);
        
        TreePrefixPass (program, node->right, graphFile);
    }
}

int DumpMakeConfig (program_t *program, node_t *node)
{
    assert (program);
    assert (node);

    imageCounter++;

    char graphFilePath[kFileNameLen + 22] = {};
    snprintf (graphFilePath, kFileNameLen + 22, "%s%lu.dot", program->log.dotFolderPath, imageCounter);

    DEBUG_VAR ("%s", graphFilePath);

    FILE *graphFile  = fopen (graphFilePath, "w");
    if (graphFile == NULL)
    {
        ERROR_LOG ("Error opening file \"%s\"", graphFilePath);

        return TREE_ERROR_COMMON |
               COMMON_ERROR_OPENING_FILE;
    }

    fprintf (graphFile  ,   "digraph G {\n"
                            // "\tsplines=ortho;\n"
                            // "\tnodesep=0.5;\n"
                            "\tnode [shape=octagon; style=\"filled\"; fillcolor=\"#ff8080\"];\n");

    TreePrefixPass (program, node, graphFile);

    fprintf (graphFile, "%s", "}");
    fclose  (graphFile);

    return TREE_OK;
}
int DumpMakeImg (node_t *node, treeLog_t *log)
{
    assert (node);
    assert (log);

    char imgFileName[kFileNameLen] = {};
    snprintf (imgFileName, kFileNameLen, "%lu.png", imageCounter);

    const size_t kMaxCommandLen = 256;
    char command[kMaxCommandLen] = {};

    snprintf (command, kMaxCommandLen, "dot %s%lu.dot -T png -o %s%s", 
              log->dotFolderPath, imageCounter,
              log->imgFolderPath, imgFileName);

    int status = system (command);
    DEBUG_VAR ("%d", status);
    if (status != 0)
    {
        ERROR_LOG ("ERROR executing command \"%s\"", command);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_RUNNING_SYSTEM_COMMAND;
    }

    fprintf (log->htmlFile,
             "<img src=\"img/%s\" hieght=\"500px\">\n",
             imgFileName);

    DEBUG_VAR ("%s", command);

    return TREE_OK;
}
