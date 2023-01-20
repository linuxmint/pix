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

#include <config.h>
#include <glib/gi18n.h>
#include "gth-progress-dialog.h"
#include "gtk-utils.h"

#define MAX_DESCRIPTION_LABEL_WIDTH_CHARS 60
#define SHOW_DELAY 500
#define PULSE_INTERVAL (1000 / 12)
#define PULSE_STEP (1.0 / 20.0)


/* -- gth_task_progress -- */


#define GTH_TYPE_TASK_PROGRESS            (gth_task_progress_get_type ())
#define GTH_TASK_PROGRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TASK_PROGRESS, GthTaskProgress))
#define GTH_TASK_PROGRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TASK_PROGRESS, GthTaskProgressClass))
#define GTH_IS_TASK_PROGRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TASK_PROGRESS))
#define GTH_IS_TASK_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TASK_PROGRESS))
#define GTH_TASK_PROGRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_TASK_PROGRESS, GthTaskProgressClass))

typedef struct _GthTaskProgress GthTaskProgress;
typedef struct _GthTaskProgressClass GthTaskProgressClass;

struct _GthTaskProgress {
	GtkBox     parent_instance;
	GthTask   *task;
	GtkWidget *description_label;
	GtkWidget *details_label;
	GtkWidget *fraction_progressbar;
	GtkWidget *cancel_button;
	gulong     task_progress;
	gulong     task_completed;
	guint      pulse_event;
};

struct _GthTaskProgressClass {
	GtkBoxClass parent_class;
};


GType gth_task_progress_get_type (void);


G_DEFINE_TYPE (GthTaskProgress, gth_task_progress, GTK_TYPE_BOX)


static void
gth_task_progress_finalize (GObject *base)
{
	GthTaskProgress *self = (GthTaskProgress *) base;

	if (self->pulse_event != 0)
		g_source_remove (self->pulse_event);
	if (self->task_progress != 0)
		g_signal_handler_disconnect (self->task, self->task_progress);
	if (self->task_completed != 0)
		g_signal_handler_disconnect (self->task, self->task_completed);
	if (self->task != NULL)
		g_object_unref (self->task);

	G_OBJECT_CLASS (gth_task_progress_parent_class)->finalize (base);
}


static void
gth_task_progress_class_init (GthTaskProgressClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = gth_task_progress_finalize;
}


static void
cancel_button_clicked_cb (GtkButton       *button,
			  GthTaskProgress *self)
{
	gth_task_cancel (self->task);
}


static void
gth_task_progress_init (GthTaskProgress *self)
{
	GtkWidget     *vbox;
	PangoAttrList *attr_list;
	GtkWidget     *image;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
	gtk_box_set_spacing (GTK_BOX (self), 15);

	self->task = NULL;
	self->task_progress = 0;
	self->task_completed = 0;
	self->pulse_event = 0;

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (self), vbox, TRUE, TRUE, 0);

	self->description_label = gtk_label_new ("");
	gtk_widget_set_halign (self->description_label, GTK_ALIGN_START);
	gtk_label_set_xalign (GTK_LABEL (self->description_label), 0.0);
	gtk_label_set_yalign (GTK_LABEL (self->description_label), 0.5);
	gtk_label_set_ellipsize (GTK_LABEL (self->description_label), PANGO_ELLIPSIZE_END);
	gtk_label_set_max_width_chars (GTK_LABEL (self->description_label), MAX_DESCRIPTION_LABEL_WIDTH_CHARS);
	gtk_label_set_width_chars (GTK_LABEL (self->description_label), MAX_DESCRIPTION_LABEL_WIDTH_CHARS);
	gtk_widget_show (self->description_label);
	gtk_box_pack_start (GTK_BOX (vbox), self->description_label, FALSE, FALSE, 0);

	self->fraction_progressbar = gtk_progress_bar_new ();
	gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (self->fraction_progressbar), PULSE_STEP);
	gtk_widget_set_margin_top (self->fraction_progressbar, 5);
	gtk_widget_show (self->fraction_progressbar);
	gtk_box_pack_start (GTK_BOX (vbox), self->fraction_progressbar, FALSE, FALSE, 0);

	self->details_label = gtk_label_new ("");
	attr_list = pango_attr_list_new ();
	pango_attr_list_insert (attr_list, pango_attr_size_new (8500));
	g_object_set (self->details_label, "attributes", attr_list, NULL);
	gtk_widget_set_halign (self->details_label, GTK_ALIGN_START);
	gtk_widget_set_margin_top (self->details_label, 2);
	gtk_widget_set_margin_start (self->details_label, 2);
	gtk_label_set_ellipsize (GTK_LABEL (self->details_label), PANGO_ELLIPSIZE_END);

	gtk_widget_show (self->details_label);
	gtk_box_pack_start (GTK_BOX (vbox), self->details_label, FALSE, FALSE, 0);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (self), vbox, FALSE, FALSE, 0);

	self->cancel_button = gtk_button_new ();
	gtk_style_context_add_class (gtk_widget_get_style_context (self->cancel_button), "gthumb-circular-button");
	gtk_widget_show (self->cancel_button);
	g_signal_connect (self->cancel_button, "clicked", G_CALLBACK (cancel_button_clicked_cb), self);
	gtk_widget_set_tooltip_text (self->cancel_button, _("Cancel operation"));
	gtk_box_pack_start (GTK_BOX (vbox), self->cancel_button, TRUE, FALSE, 0);

	image = gtk_image_new_from_icon_name ("window-close-symbolic", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image);
	gtk_container_add (GTK_CONTAINER (self->cancel_button), image);

	pango_attr_list_unref (attr_list);
}


static gboolean
task_pulse_cb (gpointer user_data)
{
	GthTaskProgress *self = user_data;

	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (self->fraction_progressbar));

	return (self->pulse_event != 0);
}


static void
task_progress_cb (GthTask    *task,
		  const char *description,
		  const char *details,
		  gboolean    pulse,
		  double      fraction,
		  gpointer    user_data)
{
	GthTaskProgress *self = user_data;

	if (description != NULL)
		gtk_label_set_text (GTK_LABEL (self->description_label), description);
	if (details != NULL)
		gtk_label_set_text (GTK_LABEL (self->details_label), details);
	if (pulse) {
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR (self->fraction_progressbar));
		if (self->pulse_event == 0)
			self->pulse_event = gdk_threads_add_timeout (PULSE_INTERVAL, task_pulse_cb, self);
	}
	else {
		if (self->pulse_event != 0) {
			g_source_remove (self->pulse_event);
			self->pulse_event = 0;
		}
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (self->fraction_progressbar), fraction);
	}
}


static void gth_progress_dialog_child_removed (GthProgressDialog *dialog);


static void
task_completed_cb (GthTask  *task,
		   GError   *error,
		   gpointer  user_data)
{
	GthTaskProgress *self = user_data;
	GtkWindow       *toplevel;

	if (self->pulse_event != 0) {
		g_source_remove (self->pulse_event);
		self->pulse_event = 0;
	}

	toplevel = _gtk_widget_get_toplevel_if_window (GTK_WIDGET (self));

	gtk_widget_destroy (GTK_WIDGET (self));

	if ((toplevel != NULL) && GTH_IS_PROGRESS_DIALOG (toplevel))
		gth_progress_dialog_child_removed (GTH_PROGRESS_DIALOG (toplevel));
}


GtkWidget *
gth_task_progress_new (GthTask *task)
{
	GthTaskProgress *self;

	self = g_object_new (GTH_TYPE_TASK_PROGRESS, NULL);
	self->task = g_object_ref (task);
	self->task_progress = g_signal_connect (self->task,
						"progress",
						G_CALLBACK (task_progress_cb),
						self);
	self->task_completed = g_signal_connect (self->task,
						 "completed",
						 G_CALLBACK (task_completed_cb),
						 self);

	return (GtkWidget *) self;
}


/* -- gth_progress_dialog -- */


struct _GthProgressDialogPrivate {
	GtkWindow *parent;
	GtkWidget *task_box;
	gulong     show_event;
	gboolean   custom_dialog_opened;
	gboolean   destroy_with_tasks;
};


G_DEFINE_TYPE_WITH_CODE (GthProgressDialog,
			 gth_progress_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (GthProgressDialog))


static void
gth_progress_dialog_finalize (GObject *base)
{
	GthProgressDialog *self = (GthProgressDialog *) base;

	if (self->priv->show_event != 0) {
		g_source_remove (self->priv->show_event);
		self->priv->show_event = 0;
	}

	G_OBJECT_CLASS (gth_progress_dialog_parent_class)->finalize (base);
}


static void
gth_progress_dialog_class_init (GthProgressDialogClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = gth_progress_dialog_finalize;
}


static void
progress_dialog_response_cb (GtkDialog *dialog,
			     int        response,
			     gpointer   user_data)
{
	if (response == GTK_RESPONSE_CLOSE)
		gtk_widget_hide (GTK_WIDGET (dialog));
}


static void
gth_progress_dialog_init (GthProgressDialog *self)
{
	self->priv = gth_progress_dialog_get_instance_private (self);
	self->priv->custom_dialog_opened = FALSE;
	self->priv->destroy_with_tasks = FALSE;

	gtk_window_set_title (GTK_WINDOW (self), _("Operations"));
	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);

	gtk_dialog_add_button (GTK_DIALOG (self), _GTK_LABEL_CLOSE, GTK_RESPONSE_CLOSE);

	self->priv->task_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 15);
	gtk_widget_show (self->priv->task_box);
	gtk_container_set_border_width (GTK_CONTAINER (self->priv->task_box), 15);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), self->priv->task_box, FALSE, FALSE, 0);

	g_signal_connect (self, "response", G_CALLBACK (progress_dialog_response_cb), self);
	g_signal_connect (self, "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), self);
}


GtkWidget *
gth_progress_dialog_new (GtkWindow *parent)
{
	GthProgressDialog *self;

	self = g_object_new (GTH_TYPE_PROGRESS_DIALOG,
			     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
			     NULL);
	self->priv->parent = parent;

	return (GtkWidget *) self;
}


void
gth_progress_dialog_destroy_with_tasks	(GthProgressDialog	*self,
					 gboolean		 value)
{
	g_return_if_fail (GTH_IS_PROGRESS_DIALOG (self));
	self->priv->destroy_with_tasks = value;
}


static gboolean
_show_dialog_cb (gpointer data)
{
	GthProgressDialog *self = data;

	if (self->priv->show_event != 0) {
		g_source_remove (self->priv->show_event);
		self->priv->show_event = 0;
	}

	if (! self->priv->custom_dialog_opened && (_gtk_container_get_n_children (GTK_CONTAINER (self->priv->task_box)) > 0)) {
		gtk_window_set_transient_for (GTK_WINDOW (self), self->priv->parent);
		gtk_window_present (GTK_WINDOW (self));
	}

	return FALSE;
}


static void
progress_dialog_task_dialog_cb (GthTask   *task,
				gboolean   opened,
				GtkWidget *dialog,
				gpointer   user_data)
{
	GthProgressDialog *self = user_data;

	self->priv->custom_dialog_opened = opened;
	if (opened) {
		if (self->priv->show_event != 0) {
			g_source_remove (self->priv->show_event);
			self->priv->show_event = 0;
		}
		gtk_widget_hide (GTK_WIDGET (self));
		gtk_window_set_transient_for (GTK_WINDOW (self), NULL);
		if (dialog != NULL)
			gtk_window_set_transient_for (GTK_WINDOW (dialog), self->priv->parent);
	}
	else if (self->priv->show_event == 0)
		self->priv->show_event = g_timeout_add (SHOW_DELAY, _show_dialog_cb, self);
}


static void
progress_dialog_task_progress_cb (GthTask    *task,
				  const char *description,
				  const char *details,
				  gboolean    pulse,
				  double      fraction,
				  gpointer    user_data)
{
	GthProgressDialog *self = user_data;
	GString           *title;

	/* set the task description as dialog title if the main window is hidden */

	if (description == NULL)
		return;

	if ((self->priv->parent != NULL) && gtk_widget_get_mapped (GTK_WIDGET (self->priv->parent)))
		return;

	title = g_string_new ("");
	g_string_append (title, description);
	g_string_append (title, " - ");
	g_string_append (title, _("gThumb"));
	gtk_window_set_title (GTK_WINDOW (self), title->str);

	g_string_free (title, TRUE);
}


static void
progress_dialog_task_completed_cb (GthTask  *task,
		   	   	   GError   *error,
		   	   	   gpointer  user_data)
{
	GthProgressDialog *self = user_data;

	if ((error != NULL) && ! g_error_matches (error, GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED))
		_gtk_error_dialog_from_gerror_show (self->priv->parent,
						    _("Could not perform the operation"),
						    error);
}


void
gth_progress_dialog_add_task (GthProgressDialog *self,
			      GthTask           *task,
			      GthTaskFlags       flags)
{
	GtkWidget *child;

	g_signal_connect (task,
			  "dialog",
			  G_CALLBACK (progress_dialog_task_dialog_cb),
			  self);
	g_signal_connect (task,
			  "progress",
			  G_CALLBACK (progress_dialog_task_progress_cb),
			  self);
	if ((flags & GTH_TASK_FLAGS_IGNORE_ERROR) == 0)
		g_signal_connect (task,
				  "completed",
				  G_CALLBACK (progress_dialog_task_completed_cb),
				  self);

	gtk_window_set_title (GTK_WINDOW (self), _("Operations"));

	child = gth_task_progress_new (task);
	gtk_widget_show (child);
	gtk_box_pack_start (GTK_BOX (self->priv->task_box), child, TRUE, TRUE, 0);
	gth_task_exec (task, NULL);

	if ((self->priv->parent == NULL) || ! gtk_widget_get_mapped (GTK_WIDGET (self->priv->parent)))
		_show_dialog_cb (self);
	else if (self->priv->show_event == 0)
		self->priv->show_event = g_timeout_add (SHOW_DELAY, _show_dialog_cb, self);
}


static void
gth_progress_dialog_child_removed (GthProgressDialog *self)
{
	if (_gtk_container_get_n_children (GTK_CONTAINER (self->priv->task_box)) == 0) {
		if (self->priv->show_event != 0) {
			g_source_remove (self->priv->show_event);
			self->priv->show_event = 0;
		}
		if (self->priv->destroy_with_tasks)
			gtk_widget_destroy (GTK_WIDGET (self));
		else
			gtk_widget_hide (GTK_WIDGET (self));
	}
}
