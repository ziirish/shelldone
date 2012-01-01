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
#ifndef _JOBS_H_
#define _JOBS_H_

#include "structs.h"

typedef struct _jobs jobs;
typedef struct _job job;

/* A job is simply a command */
struct _job
{
    command *content;

    job *next;
    job *prev;
};

struct _jobs
{
    job *head;
    job *tail;
    int size;
};

/**
 * Add a job to the job's list
 * @param ptr Command to add to the job list
 */
void enqueue_job (command *ptr, unsigned int stopped);

/**
 * List running jobs or finished jobs
 * @param print If TRUE prints running jobs
 */
void list_jobs (unsigned int print, int *pids, int cpt, unsigned int details);

/* Initialize jobs list */
void init_jobs (void);

/* Clear jobs list */
void clear_jobs (void);

/**
 * Get a job by its job id
 * @param j The job id
 * @return The job corresponding to this job id
 */
job *get_job_by_job_id (int j);

/**
 * Return the last enqueued job that has been stopped by SIGTSTP (^Z)
 * @param flush If TRUE, remove the job from the job queue (when we continue it
 * in foreground for instance)
 * @return The last job enqueued
 */
command *get_last_enqueued_job (unsigned int flush);

/**
 * Clear a job
 * @param ptr Job to clear
 */
void clear_job (job *ptr);

/**
 * Return the index of the job of the given pid
 * @param pid PID of the job
 * @return Index of the job
 */
int index_of (pid_t pid);

/**
 * Return the job of the given index
 * @param i Index of the job
 * @return The job
 */
job * get_job (int i);

/**
 * Return the last job in the list
 * @return The last job
 */
job * get_last_job (void);

/**
 * Remove a job
 * @param i Index of the job
 */
void remove_job (int i);

#endif
