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
#include <stdlib.h>
#include <unistd.h>

#include "list.h"
#include "xutils.h"

/* function to append an element into a given list */
void
list_append (sdlist **ptr, sddata *data)
{
    if (*ptr != NULL && data != NULL)
    {
        /* here is the first element */
        if ((*ptr)->tail == NULL)
        {
            (*ptr)->head = data;
            (*ptr)->tail = data;
            data->next = NULL;
            data->prev = NULL;
        }
        else
        {
            /* okay so our list isn't empty we append our data at the end */
            (*ptr)->tail->next = data;
            data->prev = (*ptr)->tail;
            (*ptr)->tail = data;
            data->next = NULL;
        }
        (*ptr)->size++;
    }
}

/* remove an element based on its index */
void
list_remove_id (sdlist **ptr, int idx, free_c free_content)
{
    /* idx < 0 or idx > sizeof(list) means we won't remove anything */
    if (idx < 0 || idx > (*ptr)->size)
        return;
    /* first of all we need to locate the element to remove */
    sddata *tmp = (*ptr)->head;
    int cpt = 0;
    while (tmp != NULL && cpt < idx)
    {
        tmp = tmp->next;
        cpt++;
    }
    /* maybe there is no such element */
    if (tmp != NULL)
    {
        /* we're the only one element */
        if (tmp->next == NULL && tmp->prev == NULL)
        {
            (*ptr)->head = NULL;
            (*ptr)->tail = NULL;
        }
        /* we're the last element */
        else if (tmp->next == NULL)
        {
            (*ptr)->tail = tmp->prev;
            (*ptr)->tail->next = NULL;
        }
        /* we're the first element */
        else if (tmp->prev == NULL)
        {
            (*ptr)->head = tmp->next;
            (*ptr)->head->prev = NULL;
        }
        /* we're at a random place */
        else
        {
            tmp->prev->next = tmp->next;
            tmp->next->prev = tmp->prev;
        }
        /* if we passed a command to free the ressource's content, use it */
        if (free_content != NULL)
            free_content (tmp->content);
        /* free the ressource /!\ the content cannot be freeed after this */
        xfree (tmp);
        (*ptr)->size--;
    }
}

/* returns a list of id where the passed content has been found */
int *
list_get_all_id (sdlist *ptr, void *content, eval_c eval_content)
{
    int *ret = NULL;
    if (ptr != NULL && eval_content != NULL)
    {
        sddata *tmp = ptr->head;
        int cpt = 0;
        /* first iteration in order to count the number of elements */
        while (tmp != NULL)
        {
            if (eval_content (tmp->content, content) == 0)
                cpt++;
            tmp = tmp->next;
        }
        /* we found at least one element */
        if (cpt > 0)
        {
            int i = 0, j = 0;
            ret = xcalloc (cpt + 1, sizeof (int));
            tmp = ptr->head;
            /* second iteration to store content's position(s) */
            while (tmp != NULL)
            {
                if (eval_content (tmp->content, content) == 0)
                {
                    ret[i] = j;
                    i++;
                }
                tmp = tmp->next;
                j++;
            }
            ret[i] = -1;
        }
    }
    return ret;
}

/* returns the first/last position of a given content */
int
list_get_id (sdlist *ptr,
             void *content,
             eval_c eval_content,
             unsigned int last)
{
    int ret = -1;
    if (ptr != NULL && eval_content != NULL)
    {
        sddata *tmp = last ? ptr->tail : ptr->head;
        int cpt = 0;
        while (tmp != NULL)
        {
            if (eval_content (tmp->content, content) == 0)
            {
                ret = cpt;
                break;
            }
            tmp = last ? tmp->prev : tmp->next;
            cpt++;
        }
    }
    return ret;
}

/* returns an element by its id */
sddata *
list_get_data_by_id (sdlist *ptr, int id)
{
    sddata *ret = NULL;
    if (ptr != NULL && id > 0 && id < ptr->size)
    {
        int cpt = 0;
        ret = ptr->head;
        while (ret != NULL && cpt < id)
        {
            ret = ret->next;
            cpt++;
        }
        if (cpt < id)
            return NULL;
    }
    return ret;
}
