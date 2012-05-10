/**
 * Shelldone
 *
 * vim:ts=4:sw=4:expandtab
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
#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include <sys/types.h>

/* Structure that represents a command */
typedef struct _command_line command_line;

typedef struct _command command;

/** 
 * Structure that represents a command-line
 * A command-line can contains multiple command linked by operators or not
 * Thus the data-structure used for the command-line is a double liked-list of 
 * commands (cf. previous struct)
 */
typedef struct _line input_line;

/* Different flags that describe in which context the command should be run */
typedef enum {
    /* the command is followed by a '|' */
    PIPE = 1,
    /* the command is followed by a '&' */
    BG,
    /* the command is followed by a '||' */
    OR,
    /* the command is followed by a '&&' */
    AND,
    /* the command is followed by ';' or nothing (default flag) */
    END
} CmdFlag;

/* Verbosity levels */
typedef enum {
    LERROR,
    LDEBUG,
    LINFO
} log;

/* Different types of arguments protection (ie. double quote, single quote... */
typedef enum {
    NONE         = 0x01,
    DOUBLE_QUOTE = 0x02,
    SINGLE_QUOTE = 0x04,
    SETTING      = 0x08
} Protection;

struct _command {
    /* command */
    char *cmd;
    /* Protection */
    Protection protect;
    /* arguments */
    char **argv;
    /* argument protection */
    Protection *protected;
    /* arguments after parsing */
    char **argvf;
    /* nb arguments */
    int argc;
    /* nb arguments after parsing */
    int argcf;
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
    /* is the process stopped */
    unsigned int stopped;
    /* received SIGCONT */
    unsigned int continued;
    /* pid of the command */
    pid_t pid;
    /* job id */
    int job;
};

struct _command_line {
    command *content;

    command_line *next;
    command_line *prev;
};

/* a double linked list */
struct _line {
    /* head command */
    command_line *head;
    /* tail command */
    command_line *tail;
    /* nb commands */
    int size;
};

#endif
