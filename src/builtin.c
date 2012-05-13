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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xutils.h"
#include "builtin.h"
#include "parser.h"
#include "jobs.h"
#include "modules.h"
#include "command.h"
#include "sdlib/plugin.h"

#define open_filestream()                  \
    FILE *fdout = stdout, *fderr = stderr; \
do{                                        \
    if (out != STDOUT_FILENO)              \
        if (out == err)                    \
            fdout = fderr;                 \
        else                               \
            fdout = fdopen (out, "a");     \
    else                                   \
        fdout = stdout;                    \
    if (err != STDERR_FILENO)              \
        fderr = fdopen (err, "a");         \
    else                                   \
        fderr = stderr;                    \
}while(0);

#define close_filestream() do{              \
    if (out != STDOUT_FILENO)               \
        fclose (fdout);                     \
    if (err != STDERR_FILENO && err != out) \
        fclose (fderr);                     \
}while(0);

#define sd_print(...) fprintf(fdout,__VA_ARGS__)

#define sd_printerr(...) fprintf(fderr,__VA_ARGS__)

extern int ret_code;
extern command *curr;
extern char *plugindir;
extern unsigned int plugindirset;

extern int nb_found;
extern mod mods[];

int
sd_exit (int argc, char **argv, int in, int out, int err)
{
    (void) in;
    (void) out;
    (void) err;

    if (argc == 0)
        exit (ret_code);
    exit (strtoul (argv[0], NULL, 0));
}

int
sd_jobs (int argc, char **argv, int in, int out, int err)
{
    unsigned int opts = TRUE;
    int i;
    for (i = 0; i < argc; i++)
        opts = (xstrcmp (argv[i], "-l") == 0 ||
                xstrcmp (argv[i], "--long") == 0);

    if (argc == 0 || (argc > 0 && opts))
    {
        list_jobs (TRUE, NULL, 0, (argc > 0));
    }
    else
    {
        int *tmp = xcalloc (argc, sizeof (int));
        int i, j;
        unsigned int details = FALSE;
        for (i = 0, j = 0; i < argc; i++)
        {
            if (xstrcmp (argv[i], "-l") == 0 ||
                xstrcmp (argv[i], "--long") == 0)
            {
                details = TRUE;
            }
            else
            {
                tmp[j] = strtoul (argv[i], NULL, 10);
                j++;
            }
        }
        list_jobs (TRUE, tmp, j, details);
        xfree (tmp);
    }

    (void) in;
    (void) out;
    (void) err;
    return 0;
}

int
sd_module (int argc, char **argv, int in, int out, int err)
{
    open_filestream ();

    if (argc < 1)
    {
        sd_printerr ("module: missing argument\n");
        sd_print ("usage:\n\tmodule load|unload <module>\n");
        sd_print ("\tmodule show conf\n");
        sd_print ("\tmodule set param value\n");
        sd_print ("\tmodule list loaded|available\n");
        close_filestream ();
        return 1;
    }

    if (xstrcmp (argv[0], "show") == 0)
    {
        if (argc != 2)
        {
            sd_printerr ("module show: missing argument\n");
            sd_print ("usage:\n\tmodule show conf\n");
            close_filestream ();
            return 2;
        }
        if (xstrcmp (argv[1], "conf") == 0)
        {
            sd_print ("path: %s\n", plugindir);
            close_filestream ();
            return 0;
        }
    }

    if (xstrcmp (argv[0], "set") == 0)
    {
        if (argc != 3)
        {
            sd_printerr ("module set: missing argument\n");
            sd_print ("usage:\n\tmodule set param value\n");
            close_filestream ();
            return 2;
        }
        if (xstrcmp (argv[1], "path") == 0)
        {
            struct stat sb;
            if (stat (argv[2], &sb) == -1)
            {
                sd_printerr ("stat: %s\n", strerror (errno));
                close_filestream ();
                return 3;
            }
            if ((sb.st_mode & S_IFMT) != S_IFDIR)
            {
                sd_printerr ("error: '%s' is not a directory\n", argv[2]);
                close_filestream ();
                return 4;
            }
            if (plugindirset)
                xfree (plugindir);
            plugindirset = TRUE;
            plugindir = xstrdup (argv[2]);
            sd_print ("path=%s\n", plugindir);
            sd_print ("searching plugins in '%s'\n", plugindir);
            clear_modules ();
            init_modules ();
        }
    }

    if (xstrcmp (argv[0], "load") == 0)
    {
        if (argc != 2)
        {
            sd_printerr ("module load: missing argument\n");
            sd_print ("usage:\n\tmodule load <module>\n");
            close_filestream ();
            return 2;
        }
        if (!load_module_by_name (argv[1]))
        {
            sd_printerr ("module load: unable to find module '%s'\n", argv[1]);
            close_filestream ();
            return 3;
        }
    }

    if (xstrcmp (argv[0], "unload") == 0)
    {
        if (argc != 2)
        {
            sd_printerr ("module unload: missing argument\n");
            sd_print ("usage:\n\tmodule unload <module>\n");
            close_filestream ();
            return 2;
        }

        if (!is_module_present (argv[1]))
        {
            sd_printerr ("module '%s' not present\n", argv[1]);
            close_filestream ();
            return 3;
        }

        unload_module_by_name (argv[1]);
    }

    if (xstrcmp (argv[0], "list") == 0)
    {
        if (argc != 2)
        {
            sd_printerr ("module list: missing argument\n");
            sd_print ("usage:\n\tmodule list loaded|available\n");
            close_filestream ();
            return 2;
        }
        if (xstrcmp (argv[1], "loaded") == 0)
        {
            unsigned int loaded = FALSE;
            sdplist *modules = get_modules_list_by_type (PROMPT);
            if (modules != NULL)
            {
                sdplugin *tmp = modules->head;
                loaded = TRUE;
                sd_print ("=== PROMPT ===\n");
                while (tmp != NULL)
                {
                    sd_print ("%s", tmp->content->name);
                    tmp = tmp->next;
                    if (tmp != NULL)
                        sd_print (" -> ");
                }
                sd_print ("\n");
                free_sdplist (modules);
                modules = NULL;
            }
            modules = get_modules_list_by_type (PARSING);
            if (modules != NULL)
            {
                sdplugin *tmp = modules->head;
                loaded = TRUE;
                sd_print ("=== PARSING ===\n");
                while (tmp != NULL)
                {
                    sd_print ("%s", tmp->content->name);
                    tmp = tmp->next;
                    if (tmp != NULL)
                        sd_print (" -> ");
                }
                sd_print ("\n");
                free_sdplist (modules);
                modules = NULL;
            }
            modules = get_modules_list_by_type (BUILTIN);
            if (modules != NULL)
            {
                sdplugin *tmp = modules->head;
                loaded = TRUE;
                sd_print ("=== BUILTIN ===\n");
                while (tmp != NULL)
                {
                    sd_print ("%s", tmp->content->name);
                    tmp = tmp->next;
                    if (tmp != NULL)
                        sd_print (" -> ");
                }
                sd_print ("\n");
                free_sdplist (modules);
                modules = NULL;
            }
            if (loaded)
            {
                sd_print ("\
Notice: the above list is ordered by 'prio'. A higher prio means the plugin\n\
is executed lately.\n");
            }
        }
        else if (xstrcmp (argv[1], "available") == 0)
        {
            int i;
            for (i = 0; i < nb_found; i++)
                sd_print ("[%c] %20s (type: %s)\n",
                          is_module_present (mods[i].name) ? 'X' : ' ',
                          mods[i].name,
                          mods[i].type);
        }
    }

    close_filestream ();

    (void) in;

    return 0;
}

int
sd_bg (int argc, char **argv, int in, int out, int err)
{
    int ret = 0;

    open_filestream ();

    if (argc == 0)
    {
        command *tmp = get_last_enqueued_job (FALSE);
        if (tmp != NULL)
        {
            tmp->continued = TRUE;
            if (tmp->stopped)
            {
                sd_print ("[%d]  + continued %d (%s)\n",
                          tmp->job,
                          tmp->pid,
                          tmp->cmd);
                tmp->stopped = FALSE;
                kill (tmp->pid, SIGCONT);
            }
        }
        else
        {
            sd_printerr ("bg: no jobs enqeued\n");
            ret = 254;
        }
    }
    else
    {
        int i;
        for (i = 0; i < argc; i++)
        {
            int index = index_of (strtoul (argv[i], NULL, 10));
            job *tmp = get_job (index);
            if (tmp != NULL)
            {
                tmp->content->continued = TRUE;
                if (tmp->content->stopped)
                {
                    const char l = (tmp == get_last_job ()) ? '+' : '-';
                    sd_print ("[%d]  %c continued %d (%s)\n",
                              tmp->content->job,
                              l,
                              tmp->content->pid,
                              tmp->content->cmd);
                    tmp->content->stopped = FALSE;
                    kill (tmp->content->pid, SIGCONT);
                }
            }
            else
            {
                sd_printerr ("bg: '%s' no such process\n", argv[i]);
                ret_code = 254;
            }
        }
    }

    close_filestream ();

    (void) in;
    return ret;
}

int
sd_fg (int argc, char **argv, int in, int out, int err)
{
    open_filestream ();

    if (argc == 0)
    {
        command *tmp = get_last_enqueued_job (TRUE);
        if (tmp != NULL)
        {
            int status;
            int r;
            pid_t p;
            tmp->continued = TRUE;
            if (tmp->stopped)
            {
                sd_print ("[%d]  + continued %d (%s)\n",
                          tmp->job,
                          tmp->pid,
                          tmp->cmd);
                tmp->stopped = FALSE;
                kill (tmp->pid, SIGCONT);
            }
            signal (SIGTSTP, sigstophandler);
            curr = tmp;

            p = tmp->pid;

            r = waitpid (p, &status, 0);
            if (r != -1)
                ret_code = WEXITSTATUS(status);
            else
                ret_code = 254;

            free_command (tmp);
        }
        else
        {
            sd_printerr ("fg: no jobs enqeued\n");
            ret_code = 254;
        }
    }
    else
    {
        int i;
        for (i = 0; i < argc; i++)
        {
            int status;
            int r;
            int index = index_of (strtoul (argv[i], NULL, 10));
            job *tmp = get_job (index);
            if (tmp != NULL)
            {
                pid_t p;
                tmp->content->continued = TRUE;
                if (tmp->content->stopped)
                {
                    const char l = (tmp == get_last_job ()) ? '+' : '-';
                    sd_print ("[%d]  %c continued %d (%s)\n",
                              tmp->content->job,
                              l,
                              tmp->content->pid,
                              tmp->content->cmd);
                    tmp->content->stopped = FALSE;
                    kill (tmp->content->pid, SIGCONT);
                }
                signal (SIGTSTP, sigstophandler);
                curr = copy_command (tmp->content);
                remove_job (index);

                p = curr->pid;

                r = waitpid (p, &status, 0);
                if (r != -1)
                    ret_code = WEXITSTATUS(status);
                else
                    ret_code = 254;

                free_command (curr);
            }
            else
            {
                sd_printerr ("fg: '%s' no such process\n", argv[i]);
                ret_code = 254;
            }
        }
    }

    close_filestream ();
    (void) in;

    return ret_code;
}

int
sd_rehash (int argc, char **argv, int in, int out, int err)
{
    (void) argc;
    (void) argv;
    (void) in;
    (void) out;
    (void) err;

    clear_command_list ();
    init_command_list ();

    return 0;
}

int 
sd_pwd (int argc, char **argv, int in, int out, int err)
{
    char *pwd;
    open_filestream ();

    if (argc > 0)
    {
        sd_printerr ("ERROR: too many arguments\n");
        sd_printerr ("usage: pwd\n");
        close_filestream ();
/*        _exit (1); */
        return 1;
    }

    pwd = getcwd (NULL, 0);
    sd_print ("%s\n", /*getenv ("PWD")*/pwd);
    xfree (pwd);

    close_filestream ();
    (void) argv;
    (void) in;

/*    _exit (0); */
    return 0;
}

int 
sd_cd (int argc, char **argv, int in, int out, int err)
{
    char *target;
    open_filestream ();
    if (argc > 1)
    {
        sd_printerr ("ERROR: too many arguments\n");
        sd_printerr ("usage: cd [directory]\n");
        close_filestream ();
/*        _exit (1); */
        return 1;
    }
    if (argc == 0 || xstrcmp (argv[0], "~") == 0)
    {
        target = xstrdup (getenv ("HOME"));
    }
    else
    {
        if (xstrcmp (argv[0], "-") == 0)
        {
            target = xstrdup (getenv ("OLDPWD"));
        }
        else if (*argv[0] == '~')
        {
            char *home = getenv ("HOME");
            char *tmp = xmalloc (xstrlen (argv[0]));
            size_t len;
            int i = 0, j = 1;
            for (; argv[0][j] != '\0'; tmp[i] = argv[0][j],i++,j++);
            tmp[i] = '\0';
            len = xstrlen (home) + xstrlen (tmp) + 1;
            target = xmalloc (len);
            snprintf (target, len, "%s%s", home, tmp);
            xfree (tmp);
        }
        else
        {
            target = xstrdup (argv[0]);
        }
    }
    if (chdir (target) == -1)
    {
        xfree (target);
        /*perror ("cd");*/
        sd_printerr ("cd: %s\n", strerror (errno));
        close_filestream ();
/*        _exit (2); */
        return 2;
    }
    else
    {
        if (argc != 0 && xstrcmp (argv[0], "-") == 0)
        {
            char *tmp = getenv ("OLDPWD");
            setenv ("OLDPWD", getenv ("PWD"), 1);
            setenv ("PWD", tmp, 1);
            xfree (target);
/*            _exit (0); */
            close_filestream ();
            return 0;
        }
        setenv ("OLDPWD", getenv ("PWD"), 1);
        if (*target == '/')
        {
            setenv ("PWD", target, 1);
        }
        else if (xstrcmp (target, ".") != 0)
        {
            char *oldpwd = getenv ("OLDPWD"), *res;
            size_t s_old, s_tar;
            char **old_tab, **tar_tab, **full;
            int i, j;
            old_tab = xstrsplit (oldpwd, "/", &s_old);
            tar_tab = xstrsplit (target, "/", &s_tar);
            full = xcalloc ((s_old + s_tar), sizeof (char *));
            for (i = 0, j = 0; i < (int) s_old; i++, j++)
            {
                full[j] = xstrdup (old_tab[i]);
            }
            for (i = 0; i < (int) s_tar; i++, j++)
            {
                full[j] = xstrdup (tar_tab[i]);
            }
            xfree_list (old_tab, s_old);
            xfree_list (tar_tab, s_tar);
            i = 0;
            j = 0;
            while (i < (int) (s_old + s_tar))
            {
                if (xstrcmp (full[i], "..") == 0)
                {
                    j = xmax (j-1, 0);
                    xfree (full[j]);
                    full[j] = NULL;
                    xfree (full[i]);
                    full[i] = NULL;
                    i++;
                    continue;
                }
                else if (xstrcmp (full[i], ".") == 0)
                {
                    xfree (full[i]);
                    full[i] = NULL;
                    i++;
                    continue;
                }
                full[j] = full[i];
                j++;
                i++;
            }

            res = xstrjoin (full, j, "/");
            if (xstrcmp (res, "") != 0)
                setenv ("PWD", res, 1);
            else
                setenv ("PWD", "/", 1);

            xfree_list (full, j);
            xfree (res); 
        }
    }
    xfree (target);
/*    _exit (0); */
    
    close_filestream ();
    (void) in;

    return 0;
}

int
sd_echo (int argc, char **argv, int in, int out, int err)
{
    int nflag;
    int i;

    (void) in;
    (void) out;
    (void) err;

    if (argc < 1)
        return 0;
/*        _exit (0); */

    if (xstrcmp (argv[0], "-n") == 0) 
    {
        nflag = 1;
    }
    else
    {
        nflag = 0;
    }

    for (i = nflag ? 1 : 0; i < argc; i++) 
    {
        fprintf (stdout, "%s", argv[i]);
        if (i < argc - 1)
            fprintf (stdout, " ");
    }
    if (!nflag)
        fprintf (stdout, "\n");
    else
        fflush (stdout);

/*    _exit (0); */
    return 0;
}

int
sd_exec (int argc, char **argv, int in, int out, int err)
{
    char **my_argv;
    int i;
    if (argc == 0)
        return 0;

    if (in != 0)
    {
        close (0);
        dup (in);
    }
    if (out != 1)
    {
        close (1);
        dup (out);
    }
    if (err != 2 && err != out)
    {
        close (2);
        dup (err);
    }

    my_argv = xcalloc (argc + 1, sizeof (char *));
    for (i = 0; i < argc; i++)
        my_argv[i] = argv[i];
    my_argv[i] = NULL;

    execvp (my_argv[0], my_argv);
    fprintf (stderr, "exec: %s\n", strerror (errno));

    return 1;
}
