/*
    You can make dump more detailed by using "-D GRAPH_DETAILED"
*/

#ifndef K_TREE_LOG_H
#define K_TREE_LOG_H

struct node_t;
struct tree_t;
struct variable_t;
struct program_t;

const char kParentDumpFolderName[] = "dump/";
const char kImgFolderName[]        = "img/";
const char kDotFolderName[]        = "dot/";
const char kHtmlFileName[]         = "log.html";
const char kGraphFileName[]        = "dot.txt";

const size_t kLogFolderPathLen       = 44;
const size_t kFileNameLen            = 64;

struct treeLog_t
{
    char logFolderPath      [kLogFolderPathLen] = {}; // dump/[date-time]
    char imgFolderPath      [kFileNameLen]      = {}; // dump/[date-time]/img/
    char dotFolderPath      [kFileNameLen]      = {}; // dump/[date-time]/dot/
    char htmlFilePath       [kFileNameLen]      = {}; // dump/[date-time]/log.html

    FILE *htmlFile  = NULL;
    FILE *latexFile = NULL;
};

int LogCtor                     (treeLog_t *log);
void LogDtor                    (treeLog_t *log);
int TreeDump                    (program_t *program, tree_t *tree, 
                                 const char *file, int line, const char *func, 
                                 const char *format, ...)
                                __attribute__ ((format (printf, 6, 7)));
int NodeDump                    (program_t *program, node_t *node,
                                 const char *file, int line, const char *func, 
                                 const char *format, ...)
                                __attribute__ ((format (printf, 6, 7)));

#endif // K_TREE_LOG_H