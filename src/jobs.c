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
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <err.h>
#include <stdio.h>

#include "jobs.h"
#include "xutils.h"
#include "command.h"
#include "list.h"

static jobs *list;

static void
clear_job (job *ptr)
{
    if (ptr != NULL)
    {
        free_command (ptr->content);
        xfree (ptr);
    }
}

void
init_jobs (void)
{
    list = xmalloc (sizeof (*list));
    if (list != NULL)
    {
        list->head = NULL;
        list->tail = NULL;
        list->size = 0;
    }
}

void
clear_jobs (void)
{
    job *tmp = list->head;
    while (tmp != NULL)
    {
        job *tmp2 = tmp->next;
        clear_job (tmp);
        tmp = tmp2;
    }
    xfree (list);
}

static job *
new_job (command *ptr)
{
    job *ret = xmalloc (sizeof (*ret));
    if (ret != NULL)
    {
        ret->content = copy_command (ptr);
        ret->next = NULL;
        ret->prev = NULL;
    }
    return ret;
}

static int
generate_job_number (void)
{
    int cpt = 1;
    job *tmp = list->head;
    while (tmp != NULL)
    {
        if (tmp->content->job == cpt)
            cpt++;
        tmp = tmp->next;
    }
    return cpt;
}

void
enqueue_job (command *ptr)
{
    job *tmp = new_job (ptr);
    tmp->content->job = generate_job_number ();
    list_append ((sdlist **)&list, (sddata *)tmp);
    fprintf (stdout, "[%d] %d (%s)\n", tmp->content->job, ptr->pid, ptr->cmd);
}

static int
index_of (pid_t pid)
{
    job *tmp = list->head;
    int i = 0;
    while (tmp != NULL)
    {
        if (tmp->content->pid == pid)
            break;
        tmp = tmp->next;
        i++;
    }
    if (tmp == NULL)
        return -1;
    return i;
}

static job *
get_job (int i)
{
    if (i < 0)
        return NULL;
    job *tmp = list->head;
    int cpt = 0;
    while (tmp != NULL && cpt < i)
    {
        tmp = tmp->next;
        cpt++;
    }
    return tmp;
}

static void
remove_job (int i)
{
    if (i < 0)
        return;
    job *tmp = list->head;
    int cpt = 0;
    while (tmp != NULL && cpt < i)
    {
        tmp = tmp->next;
        cpt++;
    }
    if (tmp != NULL)
    {
        if (tmp->next == NULL && tmp->prev == NULL)
        {
            list->head = NULL;
            list->tail = NULL;
        }
        else if (tmp->next == NULL)
        {
            list->tail = tmp->prev;
            list->tail->next = NULL;
        }
        else if (tmp->prev == NULL)
        {
            list->head = tmp->next;
            list->head->prev = NULL;
        }
        else
        {
            tmp->prev->next = tmp->next;
            tmp->next->prev = tmp->prev;
        }
        free_command (tmp->content);
        xfree (tmp);
        list->size--;
    }
}

static unsigned int
is_job_done (pid_t pid)
{
    int status;
    pid_t p = waitpid (pid, &status, WNOHANG);
    if (p == -1)
    {
        int idx = index_of (pid);
        remove_job (idx);
        warn ("jobs");
        return TRUE;
    }
    if (p == 0)
        return FALSE;
    if (WIFEXITED(status) != 0)
    {
        int idx = index_of (pid);
        job *j = get_job (idx);
        fprintf (stdout, "[%d]  + %d (%s) terminated with status code %d\n",
                         j->content->job,
                         pid,
                         j->content->cmd,
                         WEXITSTATUS(status));
        remove_job (idx);
        return TRUE;
    }
    else if (WIFSIGNALED(status) != 0)
    {
        int idx = index_of (pid);
        job *j = get_job (idx);
        int sig = WTERMSIG(status);
        fprintf (stdout, "[%d]  + %d (%s) interrupted by signal %d (%s)\n",
                         j->content->job,
                         pid,
                         j->content->cmd,
                         sig,
                         strsignal (sig));
        remove_job (idx);
        return TRUE;
    }
    return FALSE;
}

void
list_jobs (unsigned int print)
{
    job *tmp = list->head;
    while (tmp != NULL)
    {
        job *tmp2 = tmp->next;
        if (!is_job_done (tmp->content->pid) && print)
            fprintf (stdout, "[%d]  + %d (%s) running\n",
                             tmp->content->job,
                             tmp->content->pid,
                             tmp->content->cmd);
        tmp = tmp2;
    }
}
