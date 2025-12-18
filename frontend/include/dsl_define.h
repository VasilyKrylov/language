#define cN NodeCopy (ast,         resTree)
#define cL NodeCopy (ast->left,   resTree)
#define cR NodeCopy (ast->right,  resTree)

#define dL NodeDiff (program, ast->left,  &program->ast, argument)
#define dR NodeDiff (program, ast->right, &program->ast, argument)

#define NUM_(num)                                                                       \
        NodeCtorAndFill (&program->ast, TYPE_CONST_NUM, {.number = num}, NULL, NULL)
#define NAME_(idxInNamesTable)                                                          \
        NodeCtorAndFill (&program->ast, TYPE_NAME, {.idx = idxInNamesTable}, NULL, NULL)
#define FUNC_(left, right)                                                              \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_FUNC}, left, right)
#define MAIN_(left, right)                                                              \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_MAIN}, left, right)
#define CALL_(left, right)                                                              \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_CALL}, left, right)
#define RETURN_(left, right)                                                            \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_RETURN}, left, right)
#define CONNECT_(left, right)                                                           \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_CONNECT},             \
                         left, right)
#define COMMA_(left, right)                                                             \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_COMMA},               \
                         left, right)
#define ADD_(left, right)                                                               \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_ADD},                 \
                         left, right)
#define DECLARATE_(left, right)                                                         \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_DECLARATE},           \
                         left, right)
#define ASSIGN_(left, right)                                                            \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_ASSIGN},              \
                         left, right)
#define PRINT_(left, right)                                                             \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_PRINT},               \
                         left, right)
#define IF_(left, right)                                                                \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_IF},                  \
                         left, right)
#define SUB_(left, right)                                                               \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_SUB},                 \
                         left, right)
#define MUL_(left, right)                                                               \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_MUL},                 \
                         left, right)
#define DIV_(left, right)                                                               \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_DIV},                 \
                         left, right)
#define POW_(left, right)                                                               \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_POW},                 \
                         left, right)
#define LN_(right)                                                                      \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_LN},                  \
                         NULL, right)
#define SIN_(right)                                                                     \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_SIN},                 \
                         NULL, right)
#define COS_(right)                                                                     \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_COS},                 \
                         NULL, right)
#define SH_(right)                                                                      \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_SH},                  \
                         NULL, right)
#define CH_(right)                                                                      \
        NodeCtorAndFill (&program->ast, TYPE_KEYWORD, {.idx = KEY_CH},                  \
                         NULL, right)
