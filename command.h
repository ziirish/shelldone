/**
 * Shelldone
 *
 * Copyright (c) 2011, Ziirish <mr.ziirish@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Ziirish.
 * 4. Neither the name of the author nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY Ziirish ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Ziirish BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
#ifndef _COMMAND_H_
#define _COMMAND_H_

typedef struct _command_line command;
typedef struct _line input_line;

typedef enum {
    PIPE = 1,
    BG,
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
    /* stdout */
    int out;
    /* stdin */
    int in;
    /* stderr */
    int err;
    /* file descriptor flag */
    int oflag;
    /* is it a builtin command */
    unsigned int builtin;
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
