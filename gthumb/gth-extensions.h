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

#ifndef GTH_EXTENSIONS_H
#define GTH_EXTENSIONS_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

G_BEGIN_DECLS

#define GTH_TYPE_EXTENSION            (gth_extension_get_type ())
#define GTH_EXTENSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_EXTENSION, GthExtension))
#define GTH_EXTENSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_EXTENSION, GthExtensionClass))
#define GTH_IS_EXTENSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_EXTENSION))
#define GTH_IS_EXTENSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_EXTENSION))
#define GTH_EXTENSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_EXTENSION, GthExtensionClass))

typedef struct _GthExtension GthExtension;
typedef struct _GthExtensionClass GthExtensionClass;

#define GTH_TYPE_EXTENSION_MODULE            (gth_extension_module_get_type ())
#define GTH_EXTENSION_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_EXTENSION_MODULE, GthExtensionModule))
#define GTH_EXTENSION_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_EXTENSION_MODULE, GthExtensionModuleClass))
#define GTH_IS_EXTENSION_MODULE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_EXTENSION_MODULE))
#define GTH_IS_EXTENSION_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_EXTENSION_MODULE))
#define GTH_EXTENSION_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_EXTENSION_MODULE, GthExtensionModuleClass))

typedef struct _GthExtensionModule GthExtensionModule;
typedef struct _GthExtensionModuleClass GthExtensionModuleClass;
typedef struct _GthExtensionModulePrivate GthExtensionModulePrivate;

#define GTH_TYPE_EXTENSION_DESCRIPTION            (gth_extension_description_get_type ())
#define GTH_EXTENSION_DESCRIPTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_EXTENSION_DESCRIPTION, GthExtensionDescription))
#define GTH_EXTENSION_DESCRIPTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_EXTENSION_DESCRIPTION, GthExtensionDescriptionClass))
#define GTH_IS_EXTENSION_DESCRIPTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_EXTENSION_DESCRIPTION))
#define GTH_IS_EXTENSION_DESCRIPTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_EXTENSION_DESCRIPTION))
#define GTH_EXTENSION_DESCRIPTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_EXTENSION_DESCRIPTION, GthExtensionDescriptionClass))

typedef struct _GthExtensionDescription GthExtensionDescription;
typedef struct _GthExtensionDescriptionClass GthExtensionDescriptionClass;
typedef struct _GthExtensionDescriptionPrivate GthExtensionDescriptionPrivate;

#define GTH_TYPE_EXTENSION_MANAGER            (gth_extension_manager_get_type ())
#define GTH_EXTENSION_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_EXTENSION_MANAGER, GthExtensionManager))
#define GTH_EXTENSION_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_EXTENSION_MANAGER, GthExtensionManagerClass))
#define GTH_IS_EXTENSION_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_EXTENSION_MANAGER))
#define GTH_IS_EXTENSION_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_EXTENSION_MANAGER))
#define GTH_EXTENSION_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_EXTENSION_MANAGER, GthExtensionManagerClass))

typedef struct _GthExtensionManager GthExtensionManager;
typedef struct _GthExtensionManagerClass GthExtensionManagerClass;
typedef struct _GthExtensionManagerPrivate GthExtensionManagerPrivate;

struct _GthExtension {
	GObject parent_instance;
	gboolean initialized;
	gboolean active;
};

struct _GthExtensionClass {
	GObjectClass parent_class;
	gboolean   (*open)            (GthExtension  *self,
				       GError       **error);
	void       (*close)           (GthExtension  *self);
	gboolean   (*activate)        (GthExtension  *self,
				       GError       **error);
	gboolean   (*deactivate)      (GthExtension  *self,
				       GError       **error);
	gboolean   (*is_configurable) (GthExtension  *self);
	void       (*configure)       (GthExtension  *self,
				       GtkWindow     *parent);
};

struct _GthExtensionModule {
	GthExtension parent_instance;
	GthExtensionModulePrivate *priv;
};

struct _GthExtensionModuleClass {
	GthExtensionClass parent_class;
};

struct _GthExtensionDescription {
	GObject parent_instance;
	char      *id;
	char      *name;
	char      *description;
	char     **authors;
	char      *copyright;
	char      *version;
	char      *icon_name;
	char      *url;
	char      *category;
	char      *loader_type;
	char     **loader_requires;
	char     **loader_after;
	gboolean   mandatory;
	gboolean   hidden;
	GthExtensionDescriptionPrivate *priv;
};

struct _GthExtensionDescriptionClass {
	GObjectClass parent_class;
};

struct _GthExtensionManager {
	GObject parent_instance;
	GthExtensionManagerPrivate *priv;
};

struct _GthExtensionManagerClass {
	GObjectClass parent_class;
};

GType                      gth_extension_get_type                  (void);
gboolean                   gth_extension_open                      (GthExtension  *self,
								    GError       **error);
void                       gth_extension_close                     (GthExtension  *self);
gboolean                   gth_extension_is_active                 (GthExtension  *self);
gboolean                   gth_extension_activate                  (GthExtension  *self,
							            GError       **error);
gboolean                   gth_extension_deactivate                (GthExtension  *self,
							            GError       **error);
gboolean                   gth_extension_is_configurable           (GthExtension  *self);
void                       gth_extension_configure                 (GthExtension  *self,
					        	   	    GtkWindow     *parent);

GType                      gth_extension_module_get_type           (void);
GthExtension *             gth_extension_module_new                (const char   *module_name);

GType                      gth_extension_description_get_type      (void);
GthExtensionDescription *  gth_extension_description_new           (GFile        *file);
gboolean                   gth_extension_description_is_active     (GthExtensionDescription *desc);
GthExtension *             gth_extension_description_get_extension (GthExtensionDescription *desc);

GType                      gth_extension_manager_get_type          (void);
GthExtensionManager *      gth_extension_manager_new               (void);
gboolean                   gth_extension_manager_open              (GthExtensionManager  *manager,
								    const char           *extension_name,
								    GError              **error);
gboolean                   gth_extension_manager_activate          (GthExtensionManager  *manager,
								    const char           *extension_name,
								    GError              **error);
gboolean                   gth_extension_manager_deactivate        (GthExtensionManager  *manager,
								    const char           *extension_name,
								    GError              **error);
gboolean                   gth_extension_manager_is_active         (GthExtensionManager  *manager,
								    const char           *extension_name);
GList *                    gth_extension_manager_get_extensions    (GthExtensionManager  *manager);
GthExtensionDescription *  gth_extension_manager_get_description   (GthExtensionManager  *manager,
								    const char           *extension_name);
GList *                    gth_extension_manager_order_extensions  (GthExtensionManager  *manager,
								    char                **extensions);

/* functions exported by modules */

void		gthumb_extension_activate		 (void);
void		gthumb_extension_deactivate		 (void);
gboolean	gthumb_extension_is_configurable	 (void);
void		gthumb_extension_configure		 (GtkWindow *parent);

G_END_DECLS

#endif
