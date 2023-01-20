/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "gth-hook.h"


#define GTH_HOOK_CALLBACK(x) ((GthHookCallback *)(x))
#define GET_HOOK(name)						\
	g_hash_table_lookup (hooks, (name));			\
	if (hook == NULL) {					\
		g_warning ("hook '%s' not found", (name));	\
		return;						\
	}
#define GET_HOOK_OR_NULL(name)					\
	g_hash_table_lookup (hooks, (name));			\
	if (hook == NULL) {					\
		g_warning ("hook '%s' not found", (name));	\
		return NULL;					\
	}

static GHashTable *hooks = NULL;
static gboolean    initialized = FALSE;


typedef struct {
	GHook ghook;
	int   sort_order;
} GthHookCallback;


typedef struct {
	GHookList *list;
	int        n_args;
	GMutex     mutex;
} GthHook;


static void
gth_hook_free (GthHook *hook)
{
	g_hook_list_clear (hook->list);
	g_free (hook->list);
	g_mutex_clear (&hook->mutex);
	g_free (hook);
}


void
gth_hooks_initialize (void)
{
	if (initialized)
		return;

	hooks = g_hash_table_new_full (g_str_hash,
				       g_str_equal,
				       (GDestroyNotify) g_free,
				       (GDestroyNotify) gth_hook_free);

	initialized = TRUE;
}


void
gth_hook_register (const char *name,
	           int         n_args)
{
	GthHook *hook;

	g_return_if_fail (name != NULL);
	g_return_if_fail ((n_args >= 0) || (n_args <= 4));

	if (g_hash_table_lookup (hooks, name) != NULL) {
		g_warning ("hook '%s' already registered", name);
		return;
	}

	hook = g_new0 (GthHook, 1);
	hook->list = g_new (GHookList, 1);
	g_hook_list_init (hook->list, sizeof (GthHookCallback));
	hook->n_args = n_args;
	g_mutex_init (&hook->mutex);

	g_hash_table_insert (hooks, g_strdup (name), hook);
}


gboolean
gth_hook_present (const char *name)
{
	g_return_val_if_fail (name != NULL, FALSE);

	return g_hash_table_lookup (hooks, name) != NULL;
}


static int
hook_compare_func (GHook *new_hook,
                   GHook *sibling)
{
	GthHookCallback *new_function = GTH_HOOK_CALLBACK (new_hook);
	GthHookCallback *sibling_function = GTH_HOOK_CALLBACK (sibling);

	if (new_function->sort_order < sibling_function->sort_order)
		return -1;
	else if (new_function->sort_order > sibling_function->sort_order)
		return 1;
	else
		return 0;
}


void
gth_hook_add_callback (const char *name,
		       int         sort_order,
		       GCallback   callback,
		       gpointer    data)
{
	GthHook *hook;
	GHook   *function;

	hook = GET_HOOK (name);

	function = g_hook_alloc (hook->list);
	function->func = callback;
	function->data = data;
	function->destroy = NULL;
	GTH_HOOK_CALLBACK (function)->sort_order = sort_order;

	g_hook_insert_sorted (hook->list, function, hook_compare_func);
}


void
gth_hook_remove_callback (const char *name,
			  GCallback   callback)
{
	GthHook *hook;
	GHook   *function;

	hook = GET_HOOK (name);
	function = g_hook_find_func (hook->list, TRUE, callback);
	if (function == NULL) {
		g_warning ("callback not found in hook '%s'", name);
		return;
	}
	g_hook_destroy_link (hook->list, function);
}


typedef void (*GthMarshaller0Args) (gpointer);
typedef void (*GthMarshaller1Arg)  (gpointer, gpointer);
typedef void (*GthMarshaller2Args) (gpointer, gpointer, gpointer);
typedef void (*GthMarshaller3Args) (gpointer, gpointer, gpointer, gpointer);
typedef void (*GthMarshaller4Args) (gpointer, gpointer, gpointer, gpointer, gpointer);


static void
invoke_marshaller_0 (GHook    *hook,
                     gpointer  data)
{
	((GthMarshaller0Args) hook->func) (hook->data);
}


static void
invoke_marshaller_1 (GHook    *hook,
                     gpointer  data)
{
	gpointer *marshal_data = data;

	((GthMarshaller1Arg) hook->func) (marshal_data[0], hook->data);
}


static void
invoke_marshaller_2 (GHook    *hook,
                     gpointer  data)
{
	gpointer *marshal_data = data;

	((GthMarshaller2Args) hook->func) (marshal_data[0], marshal_data[1], hook->data);
}


static void
invoke_marshaller_3 (GHook    *hook,
                     gpointer  data)
{
	gpointer *marshal_data = data;

	((GthMarshaller3Args) hook->func) (marshal_data[0], marshal_data[1], marshal_data[2], hook->data);
}


static void
invoke_marshaller_4 (GHook    *hook,
                     gpointer  data)
{
	gpointer *marshal_data = data;

	((GthMarshaller4Args) hook->func) (marshal_data[0], marshal_data[1], marshal_data[2], marshal_data[3], hook->data);
}


void
gth_hook_invoke (const char *name,
		 gpointer    first_data,
		 ...)
{
	GthHook         *hook;
	gpointer        *marshal_data;
	int              i = 0;
	va_list          args;
	GHookMarshaller  invoke_marshaller;

	hook = GET_HOOK (name);
	marshal_data = g_new0 (gpointer, hook->n_args);

	if (hook->n_args > 0) {
		marshal_data[i++] = first_data;
		va_start (args, first_data);
		while (i < hook->n_args)
			marshal_data[i++] = va_arg (args, gpointer);
		va_end (args);
	}

	switch (hook->n_args) {
	case 0:
		invoke_marshaller = invoke_marshaller_0;
		break;
	case 1:
		invoke_marshaller = invoke_marshaller_1;
		break;
	case 2:
		invoke_marshaller = invoke_marshaller_2;
		break;
	case 3:
		invoke_marshaller = invoke_marshaller_3;
		break;
	case 4:
		invoke_marshaller = invoke_marshaller_4;
		break;
	default:
		invoke_marshaller = NULL;
		break;
	}

	g_mutex_lock (&hook->mutex);
	if (invoke_marshaller != NULL)
		g_hook_list_marshal (hook->list, TRUE, invoke_marshaller, marshal_data);
	g_mutex_unlock (&hook->mutex);

	g_free (marshal_data);
}


typedef void * (*GthMarshaller0ArgsGet) (gpointer);
typedef void * (*GthMarshaller1ArgGet)  (gpointer, gpointer);
typedef void * (*GthMarshaller2ArgsGet) (gpointer, gpointer, gpointer);
typedef void * (*GthMarshaller3ArgsGet) (gpointer, gpointer, gpointer, gpointer);
typedef void * (*GthMarshaller4ArgsGet) (gpointer, gpointer, gpointer, gpointer, gpointer);


static void
invoke_marshaller_0_get (GHook    *hook,
                        gpointer  data)
{
	gpointer *marshal_data = data;

	if (marshal_data[0] == NULL)
		marshal_data[0] = ((GthMarshaller0ArgsGet) hook->func) (hook->data);
}


static void
invoke_marshaller_1_get (GHook    *hook,
                        gpointer  data)
{
	gpointer *marshal_data = data;

	if (marshal_data[1] == NULL)
		marshal_data[1] = ((GthMarshaller1ArgGet) hook->func) (marshal_data[0], hook->data);
}


static void
invoke_marshaller_2_get (GHook    *hook,
                         gpointer  data)
{
	gpointer *marshal_data = data;

	if (marshal_data[2] == NULL)
		marshal_data[2] = ((GthMarshaller2ArgsGet) hook->func) (marshal_data[0], marshal_data[1], hook->data);
}


static void
invoke_marshaller_3_get (GHook    *hook,
                         gpointer  data)
{
	gpointer *marshal_data = data;

	if (marshal_data[3] == NULL)
		marshal_data[3] = ((GthMarshaller3ArgsGet) hook->func) (marshal_data[0], marshal_data[1], marshal_data[2], hook->data);
}


static void
invoke_marshaller_4_get (GHook    *hook,
                         gpointer  data)
{
	gpointer *marshal_data = data;

	if (marshal_data[4] == NULL)
		marshal_data[4] = ((GthMarshaller4ArgsGet) hook->func) (marshal_data[0], marshal_data[1], marshal_data[2], marshal_data[3], hook->data);
}


void *
gth_hook_invoke_get (const char *name,
		     gpointer    first_data,
		     ...)
{
	GthHook         *hook;
	gpointer        *marshal_data;
	int              i = 0;
	va_list          args;
	GHookMarshaller  invoke_marshaller;
	void            *value;

	hook = GET_HOOK_OR_NULL (name);
	marshal_data = g_new0 (gpointer, hook->n_args + 1);
	marshal_data[hook->n_args] = NULL;

	if (hook->n_args > 0) {
		marshal_data[i++] = first_data;
		va_start (args, first_data);
		while (i < hook->n_args)
			marshal_data[i++] = va_arg (args, gpointer);
		va_end (args);
	}

	switch (hook->n_args) {
	case 0:
		invoke_marshaller = invoke_marshaller_0_get;
		break;
	case 1:
		invoke_marshaller = invoke_marshaller_1_get;
		break;
	case 2:
		invoke_marshaller = invoke_marshaller_2_get;
		break;
	case 3:
		invoke_marshaller = invoke_marshaller_3_get;
		break;
	case 4:
		invoke_marshaller = invoke_marshaller_4_get;
		break;
	default:
		invoke_marshaller = NULL;
		break;
	}

	g_mutex_lock (&hook->mutex);
	if (invoke_marshaller != NULL)
		g_hook_list_marshal (hook->list, TRUE, invoke_marshaller, marshal_data);
	g_mutex_unlock (&hook->mutex);

	value = marshal_data[hook->n_args];

	g_free (marshal_data);

	return value;
}
