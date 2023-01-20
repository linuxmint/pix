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

#ifndef GTH_TEST_CATEGORY_H
#define GTH_TEST_CATEGORY_H

#include <glib-object.h>
#include <gtk/gtk.h>

#define GTH_TYPE_TEST_CATEGORY         (gth_test_category_get_type ())
#define GTH_TEST_CATEGORY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_TEST_CATEGORY, GthTestCategory))
#define GTH_TEST_CATEGORY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_TEST_CATEGORY, GthTestCategoryClass))
#define GTH_IS_TEST_CATEGORY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_TEST_CATEGORY))
#define GTH_IS_TEST_CATEGORY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_TEST_CATEGORY))
#define GTH_TEST_CATEGORY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_TEST_CATEGORY, GthTestCategoryClass))

typedef struct _GthTestCategory         GthTestCategory;
typedef struct _GthTestCategoryPrivate  GthTestCategoryPrivate;
typedef struct _GthTestCategoryClass    GthTestCategoryClass;

struct _GthTestCategory
{
	GthTest __parent;
	GthTestCategoryPrivate *priv;
};

struct _GthTestCategoryClass
{
	GthTestClass __parent_class;
};

GType  gth_test_category_get_type  (void) G_GNUC_CONST;
void   gth_test_category_set       (GthTestCategory *self,
				    GthTestOp        op,
				    gboolean         negative,
				    const char      *value);

#endif /* GTH_TEST_CATEGORY_H */