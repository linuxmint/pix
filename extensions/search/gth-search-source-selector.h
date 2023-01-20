/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 The Free Software Foundation, Inc.
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

#ifndef GTH_SEARCH_SOURCE_SELECTOR_H
#define GTH_SEARCH_SOURCE_SELECTOR_H

#include <gtk/gtk.h>
#include "gth-search-source.h"

G_BEGIN_DECLS

#define GTH_TYPE_SEARCH_SOURCE_SELECTOR            (gth_search_source_selector_get_type ())
#define GTH_SEARCH_SOURCE_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SEARCH_SOURCE_SELECTOR, GthSearchSourceSelector))
#define GTH_SEARCH_SOURCE_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_SEARCH_SOURCE_SELECTOR, GthSearchSourceSelectorClass))
#define GTH_IS_SEARCH_SOURCE_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SEARCH_SOURCE_SELECTOR))
#define GTH_IS_SEARCH_SOURCE_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SEARCH_SOURCE_SELECTOR))
#define GTH_SEARCH_SOURCE_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_SEARCH_SOURCE_SELECTOR, GthSearchSourceSelectorClass))

typedef struct _GthSearchSourceSelector        GthSearchSourceSelector;
typedef struct _GthSearchSourceSelectorClass   GthSearchSourceSelectorClass;
typedef struct _GthSearchSourceSelectorPrivate GthSearchSourceSelectorPrivate;

struct _GthSearchSourceSelector {
	GtkBox parent_instance;
	GthSearchSourceSelectorPrivate * priv;
};

struct _GthSearchSourceSelectorClass {
	GtkBoxClass parent_class;

	void (*add_source)    (GthSearchSourceSelector *selector);
	void (*remove_source) (GthSearchSourceSelector *selector);
};

GType		gth_search_source_selector_get_type		(void);
GtkWidget *	gth_search_source_selector_new			(void);
GtkWidget *	gth_search_source_selector_new_with_source	(GthSearchSource		*source);
void		gth_search_source_selector_set_source		(GthSearchSourceSelector	*selector,
								 GthSearchSource		*source);
GthSearchSource *
		gth_search_source_selector_get_source		(GthSearchSourceSelector	*selector);
void		gth_search_source_selector_can_remove		(GthSearchSourceSelector	*selector,
								 gboolean			 value);
void		gth_search_source_selector_focus		(GthSearchSourceSelector	*self);

G_END_DECLS

#endif /* GTH_SEARCH_SOURCE_SELECTOR_H */
