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

#ifndef G_VALUE_HASH_H
#define G_VALUE_HASH_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * GValueHash:
 *
 * A #GValueHash contains an hash table of strings associated to #GValue
 * elements.
 */
typedef struct _GValueHash GValueHash;

GValueHash *   g_value_hash_new (void);
void           g_value_hash_ref             (GValueHash  *self);
void           g_value_hash_unref           (GValueHash  *self);
void           g_value_hash_set_boolean     (GValueHash  *self,
					     const char  *key,
					     gboolean     value);
void           g_value_hash_set_float       (GValueHash  *self,
					     const char  *key,
					     float        value);
void           g_value_hash_set_int         (GValueHash  *self,
					     const char  *key,
					     int          value);
void           g_value_hash_set_string      (GValueHash  *self,
					     const char  *key,
					     const char  *value);
void           g_value_hash_set_stringv     (GValueHash  *self,
					     const char  *key,
					     char       **value);
void           g_value_hash_set_string_list (GValueHash  *self,
					     const char  *key,
					     GList       *value);
void           g_value_hash_set_value       (GValueHash  *self,
					     const char  *key,
					     GValue      *value);
GValue *       g_value_hash_get_value       (GValueHash  *self,
					     const char  *key);
gboolean       g_value_hash_is_set          (GValueHash  *self,
					     const char  *key);
#define        g_value_hash_get_boolean(self, key) 	                     (g_value_get_boolean (g_value_hash_get_value ((self), (key))))
#define        g_value_hash_get_float(self, key)                             (g_value_get_float (g_value_hash_get_value ((self), (key))))
#define        g_value_hash_get_int(self, key)                               (g_value_get_int (g_value_hash_get_value ((self), (key))))
#define        g_value_hash_get_string(self, key)                            (g_value_get_string (g_value_hash_get_value ((self), (key))))
#define        g_value_hash_get_stringv(self, key)                           ((char **) g_value_get_boxed (g_value_hash_get_value ((self), (key))))
#define        g_value_hash_get_string_list(self, key)                       ((GList *) g_value_get_boxed (g_value_hash_get_value ((self), (key))))
#define        g_value_hash_get_boolean_or_default(self, key, _default)      (g_value_hash_is_set ((self), (key)) ? g_value_get_boolean (g_value_hash_get_value ((self), (key))) : (_default))
#define        g_value_hash_get_float_or_default(self, key, _default)        (g_value_hash_is_set ((self), (key)) ? g_value_get_float (g_value_hash_get_value ((self), (key))) : (_default))
#define        g_value_hash_get_int_or_default(self, key, _default)          (g_value_hash_is_set ((self), (key)) ? g_value_get_int (g_value_hash_get_value ((self), (key))) : (_default))
#define        g_value_hash_get_string_or_default(self, key, _default)       (g_value_hash_is_set ((self), (key)) ? g_value_get_string (g_value_hash_get_value ((self), (key))) : (_default))
#define        g_value_hash_get_stringv_or_default(self, key, _default)      (g_value_hash_is_set ((self), (key)) ? (char **) g_value_get_boxed (g_value_hash_get_value ((self), (key))) : (_default))
#define        g_value_hash_get_string_list_or_default(self, key, _default)  (g_value_hash_is_set ((self), (key)) ? (GList *) g_value_get_boxed (g_value_hash_get_value ((self), (key))) : (_default))
void           g_value_hash_unset           (GValueHash *self,
					     const char *key);
void           g_value_hash_clear           (GValueHash *self);

G_END_DECLS

#endif /* G_VALUE_HASH_H */
