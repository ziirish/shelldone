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
#ifndef _BSD_SOURCE
     #define _BSD_SOURCE
#endif

#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "sdlib/plugin.h"
#include "modules.h"
#include "xutils.h"
#include "list.h"

static sdplist *new_sdplist (void);
static sdplugindata *new_sdplugindata (void);
static sdplugin *new_sdplugin (void);
static void unload_all_modules (void);
static int module_equals (void *c1, void *c2);
static void free_sdplugindata (sdplugindata *ptr);
static void free_sdplugin (sdplugin *ptr);

static sdplist *modules_list = NULL;
static int nb_modules = 255;

int nb_found = 0;
mod mods[255];

extern char *plugindir;

void
init_modules (void)
{
    struct dirent **files = NULL;
    int nbfiles = scandir (plugindir, &files, NULL, NULL);
    int i, j;
    xdebug (NULL);
    modules_list = new_sdplist ();
    for (i = 0; i < nbfiles; i++)
    {
        if (S_ISDIR(DTTOIF(files[i]->d_type)) && 
            xstrcmp (files[i]->d_name, ".") != 0 &&
            xstrcmp (files[i]->d_name, "..") != 0)
        {
            struct dirent **subfiles = NULL;
            int nbsubfiles;
            char *path;
            size_t len = xstrlen (plugindir) + xstrlen (files[i]->d_name) + 2;
            path = xmalloc (len * sizeof (char));
            snprintf (path, len, "%s/%s", plugindir, files[i]->d_name);
            nbsubfiles = scandir (path, &subfiles, NULL, NULL);
            xfree (path);
            for (j = 0; j < nbsubfiles; j++)
            {
                if (nb_found < nb_modules &&
                    S_ISDIR(DTTOIF(subfiles[j]->d_type)) &&
                    xstrcmp (subfiles[j]->d_name, ".") != 0 &&
                    xstrcmp (subfiles[j]->d_name, "..") != 0)
                {
                    mods[nb_found].type = xstrdup (files[i]->d_name);
                    mods[nb_found].name = xstrdup (subfiles[j]->d_name);
                    nb_found++;
                }
                xfree (subfiles[j]);
            }
            xfree (subfiles);
        }
        xfree (files[i]);
    }
    xfree (files);
}

void
clear_modules (void)
{
    int i;
    xdebug (NULL);
    unload_all_modules ();
    free_sdplist (modules_list);
    
    for (i = 0; i < nb_found; i++)
    {
        xfree (mods[i].type);
        xfree (mods[i].name);
        mods[i].name = NULL;
        mods[i].type = NULL;
    }
    nb_found = 0;
}

static void
unload_all_modules (void)
{
    sdplugin *tmp = modules_list->head;
    while (tmp != NULL)
    {
        sdplugin *tmp2 = tmp->next;
        if (tmp->content->loaded)
        {
            unload_module (tmp->content);
        }
        tmp = tmp2;
    }
}

static sdplist *
new_sdplist (void)
{
    sdplist *ret = xcalloc (1, sizeof(*ret));
    ret->head = NULL;
    ret->tail = NULL;
    ret->size = 0;

    return ret;
}

static sdplugindata *
new_sdplugindata (void)
{
    sdplugindata *ret = xcalloc (1, sizeof (*ret));

    ret->name = NULL;
    ret->prio = 0;
    ret->loaded = FALSE;
    ret->type = UNKNOWN;
    ret->init = NULL;
    ret->clean = NULL;
    ret->main = NULL;
    ret->lib = NULL;

    return ret;
}

static sdplugin *
new_sdplugin (void)
{
    sdplugin *ret = xcalloc (1, sizeof (*ret));
    ret->content = new_sdplugindata ();

    ret->next = NULL;
    ret->prev = NULL;

    return ret;
}

static sdplugin *
new_sdplugin_empty (void)
{
    sdplugin *ret = xcalloc (1, sizeof (*ret));
    ret->content = NULL;

    ret->next = NULL;
    ret->prev = NULL;

    return ret;
}

static void
free_sdplugindata (sdplugindata *ptr)
{
    if (ptr != NULL)
    {
        xfree (ptr);
    }
}

static void
free_sdplugin (sdplugin *ptr)
{
    xdebug (NULL);
    if (ptr != NULL)
    {
        free_sdplugindata (ptr->content);
        xfree (ptr);
    }
}

void
free_sdplist (sdplist *ptr)
{
    if (ptr != NULL)
    {
        sdplugin *tmp = ptr->head;
        while (tmp != NULL)
        {
            sdplugin *tmp2 = tmp->next;

            free_sdplugindata (tmp->content);
            /* we remove the head of the list each time so we don't need to
             * increase our index */
            list_remove_id ((sdlist **)&ptr, 0, NULL);

            tmp = tmp2;
        }
        xfree (ptr);
    }
}

static sdplugindata *
copy_sdplugindata (sdplugindata *src)
{
    sdplugindata *ret = NULL;
    if (src != NULL)
    {
        ret = new_sdplugindata ();
        if (src->lib != NULL)
            memcpy (&(ret->lib), &(src->lib), sizeof (ret->lib));
        if (src->init != NULL)
            memcpy (&(ret->init), &(src->init), sizeof (ret->init));
        if (src->clean != NULL)
            memcpy (&(ret->clean), &(src->clean), sizeof (ret->clean));
        if (src->main != NULL)
            memcpy (&(ret->main), &(src->main), sizeof (ret->main));
        ret->name = src->name;
        ret->prio = src->prio;
        ret->type = src->type;
        ret->loaded = src->loaded;
    }
    return ret;
}

static sdplugin *
copy_sdplugin (sdplugin *src)
{
    sdplugin *ret = NULL;
    if (src != NULL)
    {
        ret = new_sdplugin_empty ();
        ret->content = copy_sdplugindata (src->content);
    }
    return ret;
}

static int
module_equals (void *c1, void *c2)
{
    sdplugindata *tmp = (sdplugindata *)c1;
    sdplugindata *tmp2 = (sdplugindata *)c2;
    return xstrcmp (tmp->name, tmp2->name);
}

unsigned int
is_module_present (const char *name)
{
    sdplugindata *tmp = new_sdplugindata ();
    int ret;
    tmp->name = name;
    ret = list_get_id ((sdlist *)modules_list, (void *)tmp, module_equals, FALSE);
    free_sdplugindata (tmp);
    return (ret != -1);
}

int
launch_each_module (sdplist *list, void **data)
{
    int r = 1;
    if (list != NULL)
    {
        sdplugin *tmp = list->head;
        while (tmp != NULL && r == 1)
        {
            r = tmp->content->main (data);
            tmp = tmp->next;
        }
    }
    return r;
}

static int
module_order (const void *m1, const void *m2)
{
    const sdplugin *p1 = * (sdplugin * const *) m1;
    const sdplugin *p2 = * (sdplugin * const *) m2;

    return (p1->content->prio - p2->content->prio);
}

sdplist *
get_modules_list_by_type (sdplugin_type type)
{
    sdplist *ret = NULL;
    sdplugin *curr = modules_list->head;
    int cpt = 0;
    while (curr != NULL)
    {
        if (curr->content->type == type)
            cpt++;
        curr = curr->next;
    }

    if (cpt < 1)
        return NULL;

    ret = new_sdplist ();
    sdplugin **tmp = xcalloc (cpt, sizeof (*tmp));
    int i = 0;

    curr = modules_list->head;
    while (curr != NULL && i < cpt)
    {
        if (curr->content->type == type)
        {
            tmp[i] = copy_sdplugin (curr);
            i++;
        }
        curr = curr->next;
    }

    qsort (tmp, cpt, sizeof (*tmp), module_order);

    for (i = 0; i < cpt; i++)
        list_append ((sdlist **)&ret, (sddata *)tmp[i]);

    xfree (tmp);

    return ret;
}

void
unload_module_by_name (const char *name)
{
    sdplugin *tmp = modules_list->head;
    int cpt = 0;
    while (tmp != NULL)
    {
        if (xstrcmp (tmp->content->name, name) == 0)
        {
            unload_module (tmp->content);
            free_sdplugindata (tmp->content);
            list_remove_id ((sdlist **)&modules_list, cpt, NULL);
            break;
        }
        tmp = tmp->next;
        cpt++;
    }
}

void
unload_module (sdplugindata *ptr)
{
    if (ptr != NULL && ptr->loaded)
    {
        if (ptr->clean != NULL)
            ptr->clean ();
        if (ptr->lib != NULL)
            dlclose (ptr->lib);
        ptr->loaded = FALSE;
    }
}

unsigned int
load_module_by_name (const char *name)
{
    char *path;
    int i;
    size_t len;
    unsigned int found = FALSE;
    for (i = 0; i < nb_found; i++)
    {
        if (xstrcmp (name, mods[i].name) == 0)
        {
            found = TRUE;
            len = (xstrlen (name) * 2) + xstrlen (plugindir) + 7 +
                  xstrlen (mods[i].type);
            break;
        }
    }
    if (!found)
        return FALSE;

    path = xmalloc (len * sizeof (char));
    snprintf (path, len, "%s/%s/%s/%s.so", plugindir, mods[i].type,
                                           name, name);
    load_module (path);
    xfree (path);

    return TRUE;
}

void
load_module (const char *path)
{
    sdplugin *pl = new_sdplugin ();
    sdplugindata *ptr = pl->content;
    void *func;
    char *error = NULL;

    ptr->lib = dlopen (path, RTLD_LAZY);
    error = dlerror ();
    if (error != NULL)
    {
        fprintf (stderr, "Unable to load the module '%s': %s\n",
                         path,
                         error);
        free_sdplugin (pl);
        return;
    }

    func = dlsym (ptr->lib, "sd_plugin_init");
    error = dlerror ();
    if (error != NULL)
    {
        fprintf (stderr,
                 "Failed to locate the 'sd_plugin_init' function: %s\n",
                 error);
        dlclose (ptr->lib);
        free_sdplugin (pl);
        return;
    }
    memcpy (&(ptr->init), &func, sizeof (ptr->init));

    func = dlsym (ptr->lib, "sd_plugin_main");
    error = dlerror ();
    if (error != NULL)
    {
        fprintf (stderr,
                 "Failed to locate the 'sd_plugin_main' function: %s\n",
                 error);
        dlclose (ptr->lib);
        free_sdplugin (pl);
        return;
    }
    memcpy (&(ptr->main), &func, sizeof (ptr->main));

    func = dlsym (ptr->lib, "sd_plugin_clean");
    error = dlerror ();
    if (error != NULL)
    {
        ptr->clean = NULL;
    }
    else
    {
        memcpy (&(ptr->clean), &func, sizeof (ptr->clean));
    }
    ptr->loaded = TRUE;

    ptr->init (ptr);
    if (is_module_present (ptr->name))
    {
        fprintf (stderr, "Module '%s' already loaded\n", ptr->name);
/*        unload_module (ptr);*/
        dlclose (ptr->lib);
        free_sdplugin (pl);
        return;
    }

    list_append ((sdlist **)&modules_list, (sddata *)pl);
    fprintf (stdout, "Module '%s' successfuly loaded.\n", ptr->name);
}
