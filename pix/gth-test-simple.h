/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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

#ifndef GTH_TEST_SIMPLE_H
#define GTH_TEST_SIMPLE_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "gth-file-data.h"
#include "gth-test.h"

G_BEGIN_DECLS

#define GTH_TYPE_TEST_SIMPLE         (gth_test_simple_get_type ())
#define GTH_TEST_SIMPLE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_TEST_SIMPLE, GthTestSimple))
#define GTH_TEST_SIMPLE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_TEST_SIMPLE, GthTestSimpleClass))
#define GTH_IS_TEST_SIMPLE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_TEST_SIMPLE))
#define GTH_IS_TEST_SIMPLE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_TEST_SIMPLE))
#define GTH_TEST_SIMPLE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_TEST_SIMPLE, GthTestSimpleClass))

typedef struct _GthTestSimple         GthTestSimple;
typedef struct _GthTestSimplePrivate  GthTestSimplePrivate;
typedef struct _GthTestSimpleClass    GthTestSimpleClass;

typedef int (*GthTestGetData) (GthTest        *test,
			       GthFileData    *file,
			       gpointer       *data,
			       GDestroyNotify *data_destroy_func);

typedef enum {
	GTH_TEST_DATA_TYPE_NONE,
	GTH_TEST_DATA_TYPE_INT,
	GTH_TEST_DATA_TYPE_SIZE,
	GTH_TEST_DATA_TYPE_STRING,
	GTH_TEST_DATA_TYPE_DATE,
	GTH_TEST_DATA_TYPE_DOUBLE
} GthTestDataType;

struct _GthTestSimple
{
	GthTest __parent;
	GthTestSimplePrivate *priv;
};

struct _GthTestSimpleClass
{
	GthTestClass __parent_class;
};

GType  gth_test_simple_get_type           (void) G_GNUC_CONST;
void   gth_test_simple_set_data_as_string (GthTestSimple *test,
					   const char    *s);
void   gth_test_simple_set_data_as_int    (GthTestSimple *test,
					   gint64         i);
void   gth_test_simple_set_data_as_size   (GthTestSimple *test,
					   guint64        i);
void   gth_test_simple_set_data_as_date   (GthTestSimple *test,
				   	   GDate         *date);
void   gth_test_simple_set_data_as_double (GthTestSimple *test,
				   	   gdouble        f);

G_END_DECLS

#endif /* GTH_TEST_SIMPLE_H */
