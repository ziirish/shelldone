#ifndef _COMMAND_H_
#define _COMMAND_H_

typedef struct _command_line command;
typedef struct _line input_line;

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
    command *next;
    /* prev cmd */
    command *prev;
};

/* a double linked list */
struct _line {
    /* head command */
    command *head;
    /* tail command */
    command *tail;
    /* nb commands */
    int nb;
};

command *new_cmd (void);

void free_cmd (command *ptr);

void free_line (input_line *ptr);

void dump_cmd (command *ptr);

void dump_line (input_line *ptr);

input_line *parse_line (const char *line);

char *read_line (const char *prompt);

void parse_command (command *ptr);

void run_line (input_line *ptr);

#endif
