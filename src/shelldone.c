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
#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <err.h>
#include <setjmp.h>
#include <getopt.h>

#include "xutils.h"
#include "parser.h"
#include "command.h"
#include "jobs.h"
#include "modules.h"
#include "structs.h"

pid_t shell_pgid;
int shell_terminal;
int shell_is_interactive;
static input_line *l = NULL;
static char *li = NULL;
unsigned int interrupted = FALSE;
unsigned int running = FALSE;
sigjmp_buf env;
int val;
log loglevel = 0;
char *plugindir;

static void shelldone_init (void);
static void shelldone_clean (void);
static void siginthandler (int signal);
static void shelldone_loop (void);

/* Initialization function */
static void
shelldone_init (void)
{
    xdebug (NULL);
    plugindir = SDPLDIR;
    /* See if we are running interactively */
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty (shell_terminal);
    /* initialiaze commands list */
    init_command_list ();
    /* initialize history */
    init_history ();
    /* register the cleanup function */
    atexit (shelldone_clean);
    /* initialize jobs list */
    init_jobs ();
    /* initialize modules list */
    init_modules ();
    /* ignoring SIGTSTP + handling SIGINT */
    struct sigaction sa;
    sa.sa_handler = siginthandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction (SIGINT, &sa, NULL) != 0)
        err (3, "sigaction");
    sa.sa_handler = SIG_IGN;
    if (sigaction (SIGTSTP, &sa, NULL) != 0)
        err (3, "sigaction");
    if (shell_is_interactive)
    {
        /* Loop until we are in the foreground */
        while (tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp ()))
            kill (- shell_pgid, SIGTTIN);
        /* Ignore interactive and job-control signals */
        /*
        signal (SIGTTIN, SIG_IGN);
        signal (SIGTTOU, SIG_IGN);
        signal (SIGCHLD, SIG_IGN);
        */
        /* Put ourselves in our own process group */
        shell_pgid = getpid ();
        if (setpgid (shell_pgid, shell_pgid) < 0)
            err (1, "Couldn't put the shell in its own process group");
        /* Grab control of the terminal */
        tcsetpgrp (shell_terminal, shell_pgid);
    }
}

/* Cleanup function */
static void
shelldone_clean (void)
{
    xdebug (NULL);
    xfree (li);
    free_line (l);
    li = NULL;
    l = NULL;
    clear_command_list ();
    clear_history ();
    clear_jobs ();
    clear_modules ();
}

/**
 * Signal handler for the signal 2 (SIGINT (^C))
 */
static void
siginthandler (int sig)
{
    (void) sig;
    interrupted = TRUE;
    if (!running)
        fprintf (stdout, "^C\n");
    else
        fprintf (stdout, "\n");
}

static void
shelldone_loop (void)
{
    while (1)
    {
        signal (SIGTSTP, SIG_IGN);
        val = sigsetjmp (env, TRUE);
        interrupted = FALSE;
        free_line (l);
        xfree (li);
        li = NULL;
        l = NULL;
        const char *pt = NULL;
        sdplist *modules = get_modules_list_by_type (PROMPT);
        if (modules != NULL)
        {
            void *data[] = {(void *)&pt};
            launch_each_module (modules, data);
            free_sdplist (modules);
        }
        else
        {
            pt = "shell> ";
        }
        list_jobs (FALSE, NULL, 0, FALSE);
        /* read the input line */
        li = read_line (pt);
        if (xstrcmp ("quit", li) == 0)
            break;
        if (xstrlen (li) > 0)
            insert_history (li);
        /* parsing the input line into a command-line structure */
        l = parse_line (li);
/*        dump_line (l);*/
        /* execute the command-line */
        running = TRUE;
        run_line (l);
        running = FALSE;
    }
}

static void
shelldone_read_args (int argc, char **argv)
{
    int c;
    /*int digit_optind = 0;*/

    while (1) {
        /*int this_option_optind = optind ? optind : 1;*/
        int option_index = 0;
        static struct option long_options[] = {
                {"dir",     required_argument, 0, 'd'},
                {"load",    required_argument, 0, 'l'},
                {"help",    no_argument      , 0, 'h'},
                {0,         0,                 0,  0 }
                };

        c = getopt_long(argc, argv, "d:l:hv?",
                        long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
        case 'd':
            plugindir = optarg;
            fprintf (stdout, "searching plugins in '%s'\n", plugindir);
            clear_modules ();
            init_modules ();
            break;

        case 'l': {
            size_t nb;
            int i;
            char **plugins = xstrsplit (optarg, ",", &nb);
            for (i = 0; i < (int) nb; i++)
                if (!load_module_by_name (plugins[i]))
                    fprintf (stderr, "Unable to load module '%s'\n",
                                     plugins[i]);
            xfree_list (plugins, nb);
            break;
        }

        case 'v':
            loglevel++;
            break;

        case '?':
        case 'h':
            fprintf (stdout, "\
Shelldone is a shell written from scratch in C. It is just a Proof of Concept\n\
to improve my programming skills and have some fun.\n\
usage:\n\
    shelldone [-d|--dir=<where are the plugins>]\n\
              [-l|--load=plugin1[,plugin2[...]]]\n\
              [-v|-vv...]\n\
              [-h|-?|--help]\n\
\n\
");
            exit (0);
            break;

        default:
            fprintf (stdout, "?? getopt returned character code 0%o ??\n", c);
        }
    }

    if (optind < argc) {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
    }
}

int
main (int argc, char **argv)
{
    /* initializing shelldone */
    shelldone_init ();

    /* reading arguments */
    shelldone_read_args (argc, argv);

    /* infinite loop waiting for commands to launch */
    shelldone_loop ();

/* avoid the 'unused variables' warning */
    (void) argc;
    (void) argv;

/* we don't need to cleanup anything since we registered the cleanup function */
    return 0;
}
