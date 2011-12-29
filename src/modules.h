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
#ifndef _MODULES_H_
#define _MODULES_H_

#include "sdlib/plugin.h"

/* Initialize modules list */
void init_modules (void);

/* Clear modules list */
void clear_modules (void);

/**
 * Execute each main function of the modules present in the list
 * @param list List of modules
 */
void launch_each_module (sdplist *list, void **data);

/**
 * Give a list of loaded modules by type
 * @param type Type of the moduless we are looking for
 * @return A list of modules of given type or NULL
 */
sdplist *get_modules_list_by_type (sdplugin_type type);

/**
 * Unload a module based on its name
 * @param name Name of the module we want to unload
 */
void unload_module_by_name (const char *name);

/**
 * Unload a module
 * @param ptr Module to unload
 */
void unload_module (sdplugindata *ptr);

/**
 * Load a module
 * @param path Path of the module to load
 */
void load_module (const char *path);

/**
 * Free sdplist
 * @param ptr List to free
 */
void free_sdplist (sdplist *ptr);

#endif
