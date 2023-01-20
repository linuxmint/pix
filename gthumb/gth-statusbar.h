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

#ifndef GTH_STATUSBAR_H
#define GTH_STATUSBAR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
	GTH_STATUSBAR_SECTION_FILE_LIST,
	GTH_STATUSBAR_SECTION_FILE
} GthStatusbarSection;

#define GTH_TYPE_STATUSBAR            (gth_statusbar_get_type ())
#define GTH_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_STATUSBAR, GthStatusbar))
#define GTH_STATUSBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_STATUSBAR, GthStatusbarClass))
#define GTH_IS_STATUSBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_STATUSBAR))
#define GTH_IS_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_STATUSBAR))
#define GTH_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_STATUSBAR, GthStatusbarClass))

typedef struct _GthStatusbar GthStatusbar;
typedef struct _GthStatusbarClass GthStatusbarClass;
typedef struct _GthStatusbarPrivate GthStatusbarPrivate;

struct _GthStatusbar {
	GtkBox parent_instance;
	GthStatusbarPrivate *priv;
};

struct _GthStatusbarClass {
	GtkBoxClass parent_class;
};

GType		gth_statusbar_get_type			(void);
GtkWidget *	gth_statusbar_new			(void);
void		gth_statusbar_set_list_info		(GthStatusbar		*statusbar,
							 const char		*text);
void		gth_statusbar_set_primary_text		(GthStatusbar		*statusbar,
							 const char		*text);
void		gth_statusbar_set_secondary_text	(GthStatusbar		*statusbar,
							 const char		*text);
void		gth_statusbar_set_secondary_text_temp	(GthStatusbar		*statusbar,
							 const char		*text);
void		gth_statusbar_set_progress		(GthStatusbar		*statusbar,
							 const char		*text,
							 gboolean		 pulse,
							 double			 fraction);
void		gth_statusbar_show_section		(GthStatusbar		*statusbar,
							 GthStatusbarSection	 section);
GtkWidget *	gth_statubar_get_action_area		(GthStatusbar		*statusbar);

G_END_DECLS

#endif /* GTH_STATUSBAR_H */
