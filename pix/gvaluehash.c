/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "gvaluehash.h"
#include "glib-utils.h"


struct _GValueHash {
	int         ref;
	GHashTable *table;
};


static void
_g_value_free (gpointer data)
{
	GValue *value = data;

	g_value_reset (value);
	g_free (value);
}


GValueHash *
g_value_hash_new (void)
{
	GValueHash *self;

	self = g_new0 (GValueHash, 1);
	self->ref = 1;
	self->table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, _g_value_free);

	return self;
}


void
g_value_hash_ref (GValueHash *self)
{
	self->ref++;
}


void
g_value_hash_unref (GValueHash *self)
{
	self->ref--;
	if (self->ref > 0)
		return;
	g_hash_table_destroy (self->table);
	g_free (self);
}


void
g_value_hash_set_boolean (GValueHash *self,
			  const char *key,
			  gboolean    b_value)
{
	GValue *priv_value;

	priv_value = g_new0 (GValue, 1);
	g_value_init (priv_value, G_TYPE_BOOLEAN);
	g_value_set_boolean (priv_value, b_value);
	g_hash_table_insert (self->table, g_strdup (key), priv_value);
}


void
g_value_hash_set_float (GValueHash *self,
			const char *key,
			float       f_value)
{
	GValue *priv_value;

	priv_value = g_new0 (GValue, 1);
	g_value_init (priv_value, G_TYPE_FLOAT);
	g_value_set_float (priv_value, f_value);
	g_hash_table_insert (self->table, g_strdup (key), priv_value);
}


void
g_value_hash_set_int (GValueHash *self,
		      const char *key,
		      int         i_value)
{
	GValue *priv_value;

	priv_value = g_new0 (GValue, 1);
	g_value_init (priv_value, G_TYPE_INT);
	g_value_set_int (priv_value, i_value);
	g_hash_table_insert (self->table, g_strdup (key), priv_value);
}


void
g_value_hash_set_string (GValueHash *self,
			 const char *key,
			 const char *s_value)
{
	GValue *priv_value;

	priv_value = g_new0 (GValue, 1);
	g_value_init (priv_value, G_TYPE_STRING);
	g_value_set_string (priv_value, s_value);
	g_hash_table_insert (self->table, g_strdup (key), priv_value);
}


void
g_value_hash_set_stringv (GValueHash  *self,
			  const char  *key,
			  char       **value)
{
	GValue *priv_value;

	priv_value = g_new0 (GValue, 1);
	g_value_init (priv_value, G_TYPE_STRV);
	g_value_set_boxed (priv_value, value);
	g_hash_table_insert (self->table, g_strdup (key), priv_value);
}


void
g_value_hash_set_string_list (GValueHash *self,
			      const char *key,
			      GList      *l_value)
{
	GValue *priv_value;

	priv_value = g_new0 (GValue, 1);
	g_value_init (priv_value, G_TYPE_STRING_LIST);
	g_value_set_boxed (priv_value, l_value);
	g_hash_table_insert (self->table, g_strdup (key), priv_value);
}


void
g_value_hash_set_value (GValueHash *self,
			const char *key,
			GValue     *value)
{
	GValue *priv_value;

	priv_value = g_new0 (GValue, 1);
	g_value_copy (value, priv_value);
	g_hash_table_insert (self->table, g_strdup (key), priv_value);
}


GValue *
g_value_hash_get_value (GValueHash *self,
			const char *key)
{
	return g_hash_table_lookup (self->table, key);
}


gboolean
g_value_hash_is_set (GValueHash *self,
		     const char *key)
{
	return (g_hash_table_lookup (self->table, key) != NULL);
}


void
g_value_hash_unset (GValueHash *self,
		    const char *key)
{
	g_hash_table_remove (self->table, key);
}


void
g_value_hash_clear (GValueHash *self)
{
	g_hash_table_remove_all (self->table);
}
