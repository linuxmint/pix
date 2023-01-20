/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2009 Free Software Foundation, Inc.
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

#ifndef GTH_TEST_H
#define GTH_TEST_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "gth-file-data.h"

G_BEGIN_DECLS

#define GTH_TEST_ERROR gth_test_error_quark ()

#define GTH_TYPE_TEST         (gth_test_get_type ())
#define GTH_TEST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_TEST, GthTest))
#define GTH_TEST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_TEST, GthTestClass))
#define GTH_IS_TEST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_TEST))
#define GTH_IS_TEST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_TEST))
#define GTH_TEST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_TEST, GthTestClass))

typedef struct _GthTest         GthTest;
typedef struct _GthTestPrivate  GthTestPrivate;
typedef struct _GthTestClass    GthTestClass;

typedef enum  {
	GTH_MATCH_NO = 0,
	GTH_MATCH_YES = 1,
	GTH_MATCH_LIMIT_REACHED = 2
} GthMatch;

typedef enum {
	GTH_TEST_OP_NONE,
	GTH_TEST_OP_EQUAL,
	GTH_TEST_OP_LOWER,
	GTH_TEST_OP_GREATER,
	GTH_TEST_OP_CONTAINS,
	GTH_TEST_OP_CONTAINS_ALL,
	GTH_TEST_OP_CONTAINS_ONLY,
	GTH_TEST_OP_STARTS_WITH,
	GTH_TEST_OP_ENDS_WITH,
	GTH_TEST_OP_MATCHES,
	GTH_TEST_OP_BEFORE,
	GTH_TEST_OP_AFTER
} GthTestOp;

struct _GthTest {
	GObject __parent;
	GthTestPrivate *priv;

	/*< protected >*/

	GthFileData **files;
	int            n_files;
	int            iterator;
};

struct _GthTestClass {
	GObjectClass __parent_class;

	/*< signals >*/

	void          (*changed)              (GthTest     *test);

	/*< virtual functions >*/

	const char *  (*get_attributes)       (GthTest     *test);
	GtkWidget *   (*create_control)       (GthTest     *test);
	gboolean      (*update_from_control)  (GthTest     *test,
					       GError     **error);
	void          (*focus_control)        (GthTest     *test);
	GthMatch      (*match)                (GthTest     *test,
			                       GthFileData *fdata);
	void          (*set_file_list)        (GthTest     *test,
					       GList       *files);
	GthFileData * (*get_next)             (GthTest     *test);
};

GQuark        gth_test_error_quark         (void);

GType         gth_test_get_type            (void) G_GNUC_CONST;
GthTest *     gth_test_new                 (void);
const char *  gth_test_get_id              (GthTest      *test);
const char *  gth_test_get_display_name    (GthTest      *test);
gboolean      gth_test_is_visible          (GthTest      *test);
const char *  gth_test_get_attributes      (GthTest      *test);
GtkWidget *   gth_test_create_control      (GthTest      *test);
gboolean      gth_test_update_from_control (GthTest      *test,
					    GError      **error);
void          gth_test_focus_control       (GthTest      *test);
void          gth_test_changed             (GthTest      *test);
GthMatch      gth_test_match               (GthTest      *test,
					    GthFileData  *fdata);
void          gth_test_set_file_list       (GthTest      *test,
					    GList        *files);
GthFileData * gth_test_get_next            (GthTest      *test);

G_END_DECLS

#endif /* GTH_TEST_H */
