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
#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "structs.h"

/* Allocate memory for a command structure */
command *new_command (void);

/* Allocate memory for a command-line structure */
command_line *new_cmd_line (void);

/** 
 * Free memory used by the given command-line
 * @param ptr Command-line that must be free'ed
 */
void free_cmd_line (command_line *ptr);

/**
 * Free memory used by the given command
 * @param ptr Command that must be free'ed
 */
void free_command (command *ptr);

/**
 * Duplicates command-line
 * @param src Command-line to duplicate
 * @return a new allocated command_line containing the same as src
 */
command_line *copy_cmd_line (const command_line *src);

/**
 * Duplicates command
 * @param src Command to duplicate
 * @return a new allocated command containing the same as src
 */
command *copy_command (const command *src);

/**
 * Parse the given command to separate the builtins from the rest and replace
 * the wildcards/variables/etc in order to execute in a subprocess for the
 * non-builtin commands.
 * @param ptr The command to parse
 */
void parse_command (command_line *ptrc);

/**
 * Execute the given input_line evaluating the command returns to set the
 * apropriate viariables
 * @param ptr Input-line to run
 */
void run_line (input_line *ptr);

/**
 * Signal handler for SIGTSTP
 * @param sig Signal received
 */
void sigstophandler (int sig);

#endif
