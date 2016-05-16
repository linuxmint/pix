/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 The Free Software Foundation, Inc.
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

#ifndef GTH_TEMPLATE_SELECTOR_H
#define GTH_TEMPLATE_SELECTOR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
	GTH_TEMPLATE_CODE_TYPE_TEXT,
	GTH_TEMPLATE_CODE_TYPE_ENUMERATOR,
	GTH_TEMPLATE_CODE_TYPE_SIMPLE,
	GTH_TEMPLATE_CODE_TYPE_DATE,
	GTH_TEMPLATE_CODE_TYPE_FILE_ATTRIBUTE
} GthTemplateCodeType;

typedef struct {
	GthTemplateCodeType  type;
	char                *description;
	char                 code;
} GthTemplateCode;

#define GTH_TYPE_TEMPLATE_SELECTOR            (gth_template_selector_get_type ())
#define GTH_TEMPLATE_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TEMPLATE_SELECTOR, GthTemplateSelector))
#define GTH_TEMPLATE_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TEMPLATE_SELECTOR, GthTemplateSelectorClass))
#define GTH_IS_TEMPLATE_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TEMPLATE_SELECTOR))
#define GTH_IS_TEMPLATE_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TEMPLATE_SELECTOR))
#define GTH_TEMPLATE_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_TEMPLATE_SELECTOR, GthTemplateSelectorClass))

typedef struct _GthTemplateSelector        GthTemplateSelector;
typedef struct _GthTemplateSelectorClass   GthTemplateSelectorClass;
typedef struct _GthTemplateSelectorPrivate GthTemplateSelectorPrivate;

struct _GthTemplateSelector {
	GtkBox parent_instance;
	GthTemplateSelectorPrivate *priv;
};

struct _GthTemplateSelectorClass {
	GtkBoxClass parent_class;

	void (*add_template)    (GthTemplateSelector *selector);
	void (*remove_template) (GthTemplateSelector *selector);
};

GType       gth_template_selector_get_type      (void);
GtkWidget * gth_template_selector_new           (GthTemplateCode      *allowed_codes,
						 int                   n_codes);
void        gth_template_selector_set_value     (GthTemplateSelector  *selector,
					         const char           *value);
char *      gth_template_selector_get_value     (GthTemplateSelector  *selector,
					         GError              **error);
void        gth_template_selector_can_remove    (GthTemplateSelector  *selector,
					         gboolean              value);

G_END_DECLS

#endif /* GTH_TEMPLATE_SELECTOR_H */
