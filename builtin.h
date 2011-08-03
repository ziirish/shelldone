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
#ifndef _BUILTIN_H_
#define _BUILTIN_H_

/* Structure representing a builtin command */
typedef struct _builtin builtin;

/* Generic signature for the builtin commands */
typedef int (* cmd_builtin) (int argc, char **argv, int in, int out, int err);

struct _builtin {
    /* key */
    char *key;
    /* callback to execute */
    cmd_builtin func;
};

/**
 * Builtin command to move into another directory
 * @param argc Number of arguments passed to the command
 * @param argv Array of strings containing the arguments passed to the command
 * @param in Descriptor of the standard input 
 * @param out Descriptor of the standard output
 * @param err Descriptor of the standard error output
 * @return 0 if the command succeed, non-zero if not
 */
int sd_cd (int argc, char **argv, int in, int out, int err);

/**
 * Builtin command to get the current directory
 * @param argc Number of arguments passed to the command
 * @param argv Array of strings containing the arguments passed to the command
 * @param in Descriptor of the standard input 
 * @param out Descriptor of the standard output
 * @param err Descriptor of the standard error output
 * @return 0 if the command succeed, non-zero if not
 */
int sd_pwd (int argc, char **argv, int in, int out, int err);

/**
 * Builtin command to display the given arguments
 * @param argc Number of arguments passed to the command
 * @param argv Array of strings containing the arguments passed to the command
 * @param in Descriptor of the standard input 
 * @param out Descriptor of the standard output
 * @param err Descriptor of the standard error output
 * @return 0 if the command succeed, non-zero if not
 */
int sd_echo (int argc, char **argv, int in, int out, int err);

/**
 * Builtin command to execute in the current context (ie. not in a subprocess)
 * the given arguments
 * @param argc Number of arguments passed to the command
 * @param argv Array of strings containing the arguments passed to the command
 * @param in Descriptor of the standard input 
 * @param out Descriptor of the standard output
 * @param err Descriptor of the standard error output
 * @return 0 if the command succeed, non-zero if not
 */
int sd_exec (int argc, char **argv, int in, int out, int err);

#endif
