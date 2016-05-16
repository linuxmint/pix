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

#ifndef GTH_EDIT_IPTC_PAGE_H
#define GTH_EDIT_IPTC_PAGE_H

#include <glib.h>
#include <glib-object.h>
#include <gthumb.h>

#define GTH_TYPE_EDIT_IPTC_PAGE         (gth_edit_iptc_page_get_type ())
#define GTH_EDIT_IPTC_PAGE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_EDIT_IPTC_PAGE, GthEditIptcPage))
#define GTH_EDIT_IPTC_PAGE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_EDIT_IPTC_PAGE, GthEditIptcPageClass))
#define GTH_IS_EDIT_IPTC_PAGE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_EDIT_IPTC_PAGE))
#define GTH_IS_EDIT_IPTC_PAGE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_EDIT_IPTC_PAGE))
#define GTH_EDIT_IPTC_PAGE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_EDIT_IPTC_PAGE, GthEditIptcPageClass))

typedef struct _GthEditIptcPage         GthEditIptcPage;
typedef struct _GthEditIptcPagePrivate  GthEditIptcPagePrivate;
typedef struct _GthEditIptcPageClass    GthEditIptcPageClass;

struct _GthEditIptcPage
{
	GtkBox __parent;
	GthEditIptcPagePrivate *priv;
};

struct _GthEditIptcPageClass
{
	GtkBoxClass __parent_class;
};

GType gth_edit_iptc_page_get_type (void) G_GNUC_CONST;

#endif /* GTH_EDIT_IPTC_PAGE_H */
