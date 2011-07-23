#ifndef _COMMAND_H_
#define _COMMAND_H_

typedef struct _command_line cmd;
typedef struct _line line;

typedef enum {
    PIPE = 1,
    WRITE,
    APPEND,
    READ,
    FREAD,
    OR,
    AND,
    END
} CmdFlag;

struct _command_line {
    /* command */
    char *cmd;
    /* arguments */
    char **argv;
    /* nb arguments */
    int argc;
    /* cmd flag */
    CmdFlag flag;
    /* next cmd */
    cmd *next;
    /* prev cmd */
    cmd *prev;
};

/* a double linked list */
struct _line {
    /* head command */
    cmd *head;
    /* tail command */
    cmd *tail;
    /* nb commands */
    int nb;
};

cmd *new_cmd (void);

void free_cmd (cmd *ptr);

void free_line (line *ptr);

void dump_cmd (cmd *ptr);

void dump_line (line *ptr);

line *parse_line (const char *line);

char *read_line (const char *prompt);

#endif
