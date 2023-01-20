/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2014 The Free Software Foundation, Inc.
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

#ifndef GTH_POINT_H
#define GTH_POINT_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

typedef struct {
	double x;
	double y;
} GthPoint;


typedef struct {
	GthPoint *p;
	int       n;
} GthPoints;


void		gth_points_init		(GthPoints	*points,
					 int		 n);
void		gth_points_dispose	(GthPoints	*points);
void		gth_points_copy		(GthPoints	*source,
					 GthPoints	*dest);
int		gth_points_add_point    (GthPoints	*points,
					 double		 x,
					 double		 y);
void            gth_points_delete_point	(GthPoints	*points,
					 int		 n_point);
void		gth_points_set_point    (GthPoints	*points,
					 int             n,
					 double		 x,
					 double		 y);
void		gth_points_set_pointv	(GthPoints	*points,
					 va_list	 args,
					 int		 n_points);

void		gth_points_array_init	(GthPoints      *points);
void		gth_points_array_dispose(GthPoints      *points);

double		gth_point_distance	(GthPoint	*p1,
					 GthPoint	*p2);

G_END_DECLS

#endif /* GTH_POINT_H */
