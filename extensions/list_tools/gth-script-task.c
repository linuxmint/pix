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
#include "gth-script-task.h"


struct _GthScriptTaskPrivate {
	GthScript *script;
	GtkWindow *parent;
	GList     *file_list;
	GList     *current;
	int        n_files;
	int        n_current;
	GPid       pid;
	guint      script_watch;
};


G_DEFINE_TYPE_WITH_CODE (GthScriptTask,
			 gth_script_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthScriptTask))


static void
gth_script_task_finalize (GObject *object)
{
	GthScriptTask *self;

	self = GTH_SCRIPT_TASK (object);
	g_object_unref (self->priv->script);
	_g_object_list_unref (self->priv->file_list);

	G_OBJECT_CLASS (gth_script_task_parent_class)->finalize (object);
}


static void _gth_script_task_exec (GthScriptTask *self);


static void
_gth_script_task_exec_next_file (GthScriptTask *self)
{
	self->priv->current = self->priv->current->next;
	self->priv->n_current++;

	if (self->priv->current == NULL)
		gth_task_completed (GTH_TASK (self), NULL);
	else
		_gth_script_task_exec (self);
}


static void
watch_script_cb (GPid     pid,
                 int      status,
                 gpointer data)
{
	GthScriptTask *self = data;
	GError        *error;

	g_spawn_close_pid (self->priv->pid);
	self->priv->pid = 0;
	self->priv->script_watch = 0;

	if (status != 0) {
		error = g_error_new (GTH_TASK_ERROR, GTH_TASK_ERROR_FAILED, _("Command exited abnormally with status %d"), status);
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	if (gth_script_for_each_file (self->priv->script))
		_gth_script_task_exec_next_file (self);
	else
		gth_task_completed (GTH_TASK (self), NULL);
}


static void
child_setup (gpointer user_data)
{
	/* detach from the tty */

	setsid ();

	/* create a process group to kill all the child processes when
	 * canceling the operation. */

	setpgid (0, 0);
}


static void
get_command_line_ready_cb (GObject      *source,
			   GAsyncResult *result,
			   gpointer      user_data)
{
	GthScriptTask *self = user_data;
	char          *command_line;
	GError        *error = NULL;
	gboolean       retval = FALSE;

	command_line = gth_script_get_command_line_finish (GTH_SCRIPT (source), result, &error);
	if (command_line != NULL) {
		char **argv;
		int    argc;

		if (gth_script_is_shell_script (self->priv->script)) {
			argv = g_new (char *, 4);
			argv[0] = "sh";
			argv[1] = "-c";
			argv[2] = command_line;
			argv[3] = NULL;
		}
		else
			g_shell_parse_argv (command_line, &argc, &argv, &error);

		if (error == NULL) {
			if (gth_script_wait_command (self->priv->script)) {
				if (g_spawn_async (NULL,
						   argv,
						   NULL,
						   G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
						   child_setup,
						   NULL,
						   &self->priv->pid,
						   &error))
				{
					self->priv->script_watch = g_child_watch_add (self->priv->pid,
										      watch_script_cb,
										      self);
					retval = TRUE;
				}
			}
			else {
				if (g_spawn_async (NULL,
						   argv,
						   NULL,
						   G_SPAWN_SEARCH_PATH,
						   NULL,
						   NULL,
						   NULL,
						   &error))
				{
					retval = TRUE;
				}
			}
		}

		g_free (argv);
	}

	g_free (command_line);

	if (g_error_matches (error, GTH_TASK_ERROR, GTH_TASK_ERROR_SKIP_TO_NEXT_FILE)) {
		_gth_script_task_exec_next_file (self);
		return;
	}

	if (! retval) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	if (gth_script_wait_command (self->priv->script))
		return;

	if (gth_script_for_each_file (self->priv->script)) {
		_gth_script_task_exec_next_file (self);
		return;
	}

	gth_task_completed (GTH_TASK (self), NULL);
}


static void
get_command_line_dialog_cb (GtkWidget *dialog,
			    gpointer   user_data)
{
	gth_task_dialog (GTH_TASK (user_data), (dialog != NULL), dialog);
}


static void
_gth_script_task_exec (GthScriptTask *self)
{
	if (gth_script_for_each_file (self->priv->script)) {
		GthFileData *file_data = self->priv->current->data;
		GList       *list;

		gth_task_progress (GTH_TASK (self),
				   gth_script_get_display_name (self->priv->script),
				   g_file_info_get_display_name (file_data->info),
				   FALSE,
				   (double) self->priv->n_current / (self->priv->n_files + 1));

		list = g_list_prepend (NULL, file_data);
		gth_script_get_command_line_async (self->priv->script,
						   self->priv->parent,
						   list,
						   (self->priv->file_list->next != NULL),
						   gth_task_get_cancellable (GTH_TASK (self)),
						   get_command_line_dialog_cb,
						   get_command_line_ready_cb,
						   self);

		g_list_free (list);
	}
	else {
		gth_task_progress (GTH_TASK (self),
				   gth_script_get_display_name (self->priv->script),
				   NULL,
				   TRUE,
				   0.0);

		gth_script_get_command_line_async (self->priv->script,
						   self->priv->parent,
						   self->priv->file_list,
						   FALSE,
						   gth_task_get_cancellable (GTH_TASK (self)),
						   get_command_line_dialog_cb,
						   get_command_line_ready_cb,
						   self);
	}
}


static void
file_info_ready_cb (GObject      *source_object,
                    GAsyncResult *result,
                    gpointer      user_data)
{
	GthScriptTask *self = user_data;
	GError        *error = NULL;

	_g_query_metadata_finish (result, &error);
	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	_gth_script_task_exec (self);
}


static void
gth_script_task_exec (GthTask *task)
{
	GthScriptTask *self;
	char          *attributes;

	g_return_if_fail (GTH_IS_SCRIPT_TASK (task));

	self = GTH_SCRIPT_TASK (task);

	attributes = gth_script_get_requested_attributes (self->priv->script);
	if (attributes != NULL) {
		_g_query_metadata_async (self->priv->file_list,
					 attributes,
					 gth_task_get_cancellable (task),
					 file_info_ready_cb,
					 self);
		g_free (attributes);
	}
	else
		_gth_script_task_exec (self);
}


static void
gth_script_task_cancelled (GthTask *task)
{
	GthScriptTask *self;

	g_return_if_fail (GTH_IS_SCRIPT_TASK (task));

	self = GTH_SCRIPT_TASK (task);

	if (self->priv->pid != 0)
		killpg (self->priv->pid, SIGTERM);
}


static void
gth_script_task_class_init (GthScriptTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_script_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_script_task_exec;
	task_class->cancelled = gth_script_task_cancelled;
}


static void
gth_script_task_init (GthScriptTask *self)
{
	self->priv = gth_script_task_get_instance_private (self);
	self->priv->pid = 0;
}


GthTask *
gth_script_task_new (GtkWindow *parent,
		     GthScript *script,
		     GList     *file_list)
{
	GthScriptTask *self;

	self = GTH_SCRIPT_TASK (g_object_new (GTH_TYPE_SCRIPT_TASK, NULL));
	self->priv->parent = parent;
	self->priv->script = g_object_ref (script);
	self->priv->file_list = _g_object_list_ref (file_list);
	self->priv->current = self->priv->file_list;
	self->priv->n_files = g_list_length (file_list);
	self->priv->n_current = 1;

	return (GthTask *) self;
}
