/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 The Free Software Foundation, Inc.
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

#ifndef GTH_TEST_SELECTOR_H
#define GTH_TEST_SELECTOR_H

#include <gtk/gtk.h>
#include "gth-test.h"

G_BEGIN_DECLS

#define GTH_TYPE_TEST_SELECTOR            (gth_test_selector_get_type ())
#define GTH_TEST_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TEST_SELECTOR, GthTestSelector))
#define GTH_TEST_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TEST_SELECTOR, GthTestSelectorClass))
#define GTH_IS_TEST_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TEST_SELECTOR))
#define GTH_IS_TEST_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TEST_SELECTOR))
#define GTH_TEST_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_TEST_SELECTOR, GthTestSelectorClass))

typedef struct _GthTestSelector        GthTestSelector;
typedef struct _GthTestSelectorClass   GthTestSelectorClass;
typedef struct _GthTestSelectorPrivate GthTestSelectorPrivate;

struct _GthTestSelector {
	GtkBox parent_instance;
	GthTestSelectorPrivate * priv;
};

struct _GthTestSelectorClass {
	GtkBoxClass parent_class;

	void (*add_test)    (GthTestSelector *selector);
	void (*remove_test) (GthTestSelector *selector);
};

GType       gth_test_selector_get_type      (void);
GtkWidget * gth_test_selector_new           (void);
GtkWidget * gth_test_selector_new_with_test (GthTest          *test);
void        gth_test_selector_set_test      (GthTestSelector  *selector,
					     GthTest          *test);
GthTest *   gth_test_selector_get_test      (GthTestSelector  *selector,
					     GError          **error);
void        gth_test_selector_can_remove    (GthTestSelector  *selector,
					     gboolean          value);
void        gth_test_selector_focus         (GthTestSelector  *self);

G_END_DECLS

#endif /* GTH_TEST_SELECTOR_H */
