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

#include "sdlib/plugin.h"
#include "xutils.h"
#include "list.h"

sdplist *
sdplist_new (void)
{
    sdplist *ret = xcalloc (1, sizeof(*ret));
    ret->head = NULL;
    ret->tail = NULL;
    ret->size = 0;

    return ret;
}

sdplugin *
sdplugin_new (void)
{
    sdplugin *ret = xcalloc (1, sizeof (*ret));
    ret->name = NULL;
    ret->prio = 0;
    ret->loaded = FALSE;
    ret->type = 0;
    ret->init_plugin = NULL;
    ret->clean_plugin = NULL;
    ret->main_plugin = NULL;
    ret->next = NULL;
    ret->prev = NULL;
    ret->lib = NULL;

    return ret;
}

sdplist*
get_plugins_list (sdplist *ptr, sdplugin_type type)
{
    sdplist *ret = sdplist_new ();
    sdplugin *curr = ptr->head;
    while (curr != NULL)
    {
        if (curr->type == type)
            list_append ((sdlist **)&ret, (sddata *)curr);
        curr = curr->next;
    }

    return ret;
}
