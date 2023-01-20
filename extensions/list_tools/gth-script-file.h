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

#ifndef GTH_SCRIPT_FILE_H
#define GTH_SCRIPT_FILE_H

#include <glib.h>
#include "gth-script.h"

G_BEGIN_DECLS

#define GTH_TYPE_SCRIPT_FILE         (gth_script_file_get_type ())
#define GTH_SCRIPT_FILE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_SCRIPT_FILE, GthScriptFile))
#define GTH_SCRIPT_FILE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_SCRIPT_FILE, GthScriptFileClass))
#define GTH_IS_SCRIPT_FILE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_SCRIPT_FILE))
#define GTH_IS_SCRIPT_FILE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_SCRIPT_FILE))
#define GTH_SCRIPT_FILE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_SCRIPT_FILE, GthScriptFileClass))

typedef struct _GthScriptFile         GthScriptFile;
typedef struct _GthScriptFilePrivate  GthScriptFilePrivate;
typedef struct _GthScriptFileClass    GthScriptFileClass;


struct _GthScriptFile
{
	GObject __parent;
	GthScriptFilePrivate *priv;
};

struct _GthScriptFileClass
{
	GObjectClass __parent_class;

	void  (*changed) (GthScriptFile *self);
};

GthScriptFile *  gth_script_file_get                  (void);
GthScript *      gth_script_file_get_script           (GthScriptFile  *self,
						       const char     *id);
gboolean         gth_script_file_save                 (GthScriptFile  *self,
			 			       GError        **error);
GList *          gth_script_file_get_scripts          (GthScriptFile  *self);
gboolean         gth_script_file_has_script           (GthScriptFile  *self,
						       GthScript      *script);
void             gth_script_file_add                  (GthScriptFile  *self,
						       GthScript      *script);
void             gth_script_file_remove               (GthScriptFile  *self,
						       GthScript      *script);
void             gth_script_file_clear                (GthScriptFile  *self);

G_END_DECLS

#endif /* GTH_SCRIPT_FILE_H */
