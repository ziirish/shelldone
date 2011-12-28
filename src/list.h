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
#ifndef _LIST_H_
#define _LIST_H_

/* Signature of the free_content function */
typedef void (* free_c) (void *content);

/* Signature of an evaluation function for contents comparisons */
typedef int (* eval_c) (void *c1, void *c2);

/* Double-linked list */
typedef struct _sdlist sdlist;

/* Element of a double-linked list */
typedef struct _sddata sddata;

struct _sddata
{
    /* content of an element */
    void *content;

    /* next element */
    sddata *next;
    /* previous element */
    sddata *prev;
};

struct _sdlist
{
    /* first element of the list */
    sddata *head;
    /* last element of the list */
    sddata *tail;
    /* size of the list */
    int size;
};

/**
 * Append an element to a double-linked list
 * @param ptr List in which we append an element
 * @param data Element to append to the given list
 */
void list_append (sdlist **ptr, sddata *data);

/**
 * Remove an element by id
 * @param ptr The list in which we remove an element
 * @param idx The id of the element we want to remove
 * @param free_content The callback function to free the content of the element
 * we remove
 */
void list_remove_id (sdlist **ptr, int idx, free_c free_content);

/**
 * Return a list of id of a given content
 * @param ptr List in which we search content
 * @param content Element we search in the list
 * @param eval_content Callback that compares 2 contents
 * @return a list of id
 */
int *list_get_all_id (sdlist *ptr, void *content, eval_c eval_content);

/**
 * Return the first/last id of a given content
 * @param ptr The list in which we search content
 * @param content Element we search in the list
 * @param eval_content Callback that compares 2 content
 * @param last If TRUE search starting by the end of the list
 * @return The id
 */
int list_get_id (sdlist *ptr,
                 void *content,
                 eval_c eval_content,
                 unsigned int last);

/**
 * Return the element corresponding to the given id
 * @param ptr List in which we want an element
 * @param id ID of the element
 * @return The element
 */
sddata *list_get_data_by_id (sdlist *ptr, int id);

#endif
