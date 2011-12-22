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

#include "xutils.h"
#include "parser.h"
#include "command.h"
#include "jobs.h"

/**
 * XXX: is there a better way to store these variables?
 */
pid_t shell_pgid;
int shell_terminal;
int shell_is_interactive;
static char *prompt = NULL;
static char *host = NULL;
static const char *fpwd = NULL;
static input_line *l = NULL;
static char *li = NULL;
unsigned int interrupted = FALSE;
unsigned int running = FALSE;

static void shelldone_init (void);
static void shelldone_clean (void);
static void handler (int signal);
static const char *get_prompt (void);
static void shelldone_loop (void);

int ret_code;

/* Initialization function */
static void
shelldone_init (void)
{
    /* See if we are running interactively */
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty (shell_terminal);
    /* get the hostname */
    host = xmalloc (30);
    gethostname (host, 30);
    /* load the commands list */
    init_command_list ();
    /* load the history */
    init_history ();
    /* register the cleanup function */
    atexit (shelldone_clean);
    /* load jobs list */
    init_jobs ();
    /* ignoring SIGINT */
    struct sigaction sa;
    sa.sa_handler = handler;
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
    xfree (li);
    xfree (host);
    xfree (prompt);
    free_line (l);
    li = NULL;
    host = NULL;
    prompt = NULL;
    l = NULL;
    clear_command_list ();
    clear_history ();
    clear_jobs ();
}

/**
 * Signal handler for the signal 2 (SIGINT (^C))
 */
static void
handler (int sig)
{
    (void) sig;
    interrupted = TRUE;
    if (!running)
        fprintf (stdout, "^C\n");
    else
        fprintf (stdout, "\n");
}

/**
 * Gets the prompt in a hard-coded pattern like:
 * user@hostname:pwd$
 * We can imagine make it configurable in the future
 * @return The prompt
 */
static const char *
get_prompt (void)
{
    const char *user = getenv ("USER");
    const char *full_pwd = getenv ("PWD");
    const char *home = getenv ("HOME");
    char *pwd = NULL;

    size_t s_home = xstrlen (home);
    size_t s_pwd = xstrlen (full_pwd);
    size_t full;

    if (fpwd != NULL && xstrcmp (full_pwd, fpwd) == 0)
    {
        return prompt;
    }

    if (strncmp (full_pwd, home, s_home) == 0)
    {
        size_t size = s_pwd - s_home;
        int cpt = 1;
        pwd = xmalloc (size + 2);
        *pwd = '~';
        while (cpt < (int) size + 2)
        {
            pwd[cpt] = full_pwd[s_home+cpt-1];
            cpt++;
        }
        pwd[cpt-1] = '\0';
    }
    else
        pwd = xstrdup (full_pwd);

    xfree (prompt);
    full = xstrlen (user) + xstrlen (host) + xstrlen (pwd) + 4;
    prompt = xmalloc (full + 1);
    snprintf (prompt, full + 1, "%s@%s:%s$ ", user, host, pwd);
    fpwd = full_pwd;

    xfree (pwd);
    return prompt;
}

static void
shelldone_loop (void)
{
    while (1)
    {
        signal (SIGTSTP, SIG_IGN);
        interrupted = FALSE;
        free_line (l);
        xfree (li);
        li = NULL;
        l = NULL;
        const char *pt = get_prompt ();
        list_jobs (FALSE);
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

/*
        free_line (l);
        xfree (li);
        l = NULL;
        li = NULL;
*/
    }
}

int
main (int argc, char **argv)
{
    /* initializing shelldone */
    shelldone_init ();

    /* infinite loop waiting for commands to launch */
    shelldone_loop ();

/* avoid the 'unused variables' warning */
    (void) argc;
    (void) argv;

/* we don't need to cleanup anything since we registered the cleanup function */
    return 0;
}
