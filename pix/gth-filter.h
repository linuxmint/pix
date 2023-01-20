/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006-2008 Free Software Foundation, Inc.
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

#ifndef GTH_FILTER_H
#define GTH_FILTER_H

#include <glib-object.h>
#include "gth-file-data.h"
#include "gth-test-chain.h"

G_BEGIN_DECLS

#define GTH_TYPE_FILTER         (gth_filter_get_type ())
#define GTH_FILTER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_FILTER, GthFilter))
#define GTH_FILTER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_FILTER, GthFilterClass))
#define GTH_IS_FILTER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_FILTER))
#define GTH_IS_FILTER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_FILTER))
#define GTH_FILTER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_FILTER, GthFilterClass))

typedef struct _GthFilter         GthFilter;
typedef struct _GthFilterPrivate  GthFilterPrivate;
typedef struct _GthFilterClass    GthFilterClass;

typedef enum  {
	GTH_LIMIT_TYPE_NONE = 0,
	GTH_LIMIT_TYPE_FILES,
	GTH_LIMIT_TYPE_SIZE
} GthLimitType;

struct _GthFilter
{
	GthTest __parent;
	GthFilterPrivate *priv;
};

struct _GthFilterClass
{
	GthTestClass __parent_class;
};

GType             gth_filter_get_type         (void) G_GNUC_CONST;
GthFilter *       gth_filter_new              (void);
void              gth_filter_set_limit        (GthFilter     *filter,
					       GthLimitType   type,
					       goffset        value,
					       const char    *sort_name,
					       GtkSortType    sort_direction);
void              gth_filter_get_limit        (GthFilter     *filter,
					       GthLimitType  *type,
					       goffset       *value,
					       const char   **sort_name,
					       GtkSortType   *sort_direction);
void              gth_filter_set_test         (GthFilter     *filter,
					       GthTestChain  *test);
GthTestChain *    gth_filter_get_test         (GthFilter     *filter);

G_END_DECLS

#endif /* GTH_FILTER_H */
