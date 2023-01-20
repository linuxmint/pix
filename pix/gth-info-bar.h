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

#ifndef GTH_INFO_BAR_H
#define GTH_INFO_BAR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_INFO_BAR         (gth_info_bar_get_type ())
#define GTH_INFO_BAR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_INFO_BAR, GthInfoBar))
#define GTH_INFO_BAR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_INFO_BAR, GthInfoBarClass))
#define GTH_IS_INFO_BAR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_INFO_BAR))
#define GTH_IS_INFO_BAR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_INFO_BAR))
#define GTH_INFO_BAR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_INFO_BAR, GthInfoBarClass))

typedef struct _GthInfoBar         GthInfoBar;
typedef struct _GthInfoBarPrivate  GthInfoBarPrivate;
typedef struct _GthInfoBarClass    GthInfoBarClass;

struct _GthInfoBar
{
	GtkInfoBar __parent;
	GthInfoBarPrivate *priv;
};

struct _GthInfoBarClass
{
	GtkInfoBarClass __parent_class;
};

GType         gth_info_bar_get_type           (void) G_GNUC_CONST;
GtkWidget *   gth_info_bar_new                (void);
GtkWidget *   gth_info_bar_get_primary_label  (GthInfoBar   *dialog);
void          gth_info_bar_set_icon_name      (GthInfoBar   *dialog,
					       const char   *icon_name,
					       GtkIconSize   icon_size);
void          gth_info_bar_set_gicon          (GthInfoBar   *dialog,
					       GIcon        *icon,
					       GtkIconSize   icon_size);
void          gth_info_bar_set_primary_text   (GthInfoBar   *dialog,
					       const char   *primary_text);
void          gth_info_bar_set_secondary_text (GthInfoBar   *dialog,
					       const char   *secondary_text);

G_END_DECLS

#endif /* GTH_INFO_BAR_H */
