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
#include <gthumb.h>
#include <gdk/gdkkeysyms.h>
#include "gth-script.h"
#include "shortcuts.h"


static void gth_script_dom_domizable_interface_init (DomDomizableInterface *iface);
static void gth_script_gth_duplicable_interface_init (GthDuplicableInterface *iface);


enum {
	PROP_0,
	PROP_ID,
	PROP_DISPLAY_NAME,
	PROP_COMMAND,
	PROP_VISIBLE,
	PROP_SHELL_SCRIPT,
	PROP_FOR_EACH_FILE,
	PROP_WAIT_COMMAND,
	PROP_ACCELERATOR
};


struct _GthScriptPrivate {
	char            *id;
	char            *display_name;
	char            *command;
	gboolean         visible;
	gboolean         shell_script;
	gboolean         for_each_file;
	gboolean         wait_command;
	char            *accelerator;
	char            *detailed_action;
};


G_DEFINE_TYPE_WITH_CODE (GthScript,
			 gth_script,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthScript)
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
						gth_script_dom_domizable_interface_init)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_DUPLICABLE,
						gth_script_gth_duplicable_interface_init))


static DomElement*
gth_script_real_create_element (DomDomizable *base,
				DomDocument  *doc)
{
	GthScript  *self;
	DomElement *element;

	g_return_val_if_fail (DOM_IS_DOCUMENT (doc), NULL);

	self = GTH_SCRIPT (base);

	element = dom_document_create_element (doc, "script",
					       "id", self->priv->id,
					       "display-name", self->priv->display_name,
					       "command", self->priv->command,
					       "shell-script", (self->priv->shell_script ? "true" : "false"),
					       "for-each-file", (self->priv->for_each_file ? "true" : "false"),
					       "wait-command", (self->priv->wait_command ? "true" : "false"),
					       NULL);
	if (! self->priv->visible)
		dom_element_set_attribute (element, "display", "none");

	return element;
}


static void
gth_script_real_load_from_element (DomDomizable *base,
				   DomElement   *element)
{
	GthScript *self;

	g_return_if_fail (DOM_IS_ELEMENT (element));

	self = GTH_SCRIPT (base);
	g_object_set (self,
		      "id", dom_element_get_attribute (element, "id"),
		      "display-name", dom_element_get_attribute (element, "display-name"),
		      "command", dom_element_get_attribute (element, "command"),
		      "visible", (g_strcmp0 (dom_element_get_attribute (element, "display"), "none") != 0),
		      "shell-script", (g_strcmp0 (dom_element_get_attribute (element, "shell-script"), "true") == 0),
		      "for-each-file", (g_strcmp0 (dom_element_get_attribute (element, "for-each-file"), "true") == 0),
		      "wait-command", (g_strcmp0 (dom_element_get_attribute (element, "wait-command"), "true") == 0),
		      "accelerator", "",
		      NULL);
}


static GObject *
gth_script_real_duplicate (GthDuplicable *duplicable)
{
	GthScript *script = GTH_SCRIPT (duplicable);
	GthScript *new_script;

	new_script = gth_script_new ();
	g_object_set (new_script,
		      "id", script->priv->id,
		      "display-name", script->priv->display_name,
		      "command", script->priv->command,
		      "visible", script->priv->visible,
		      "shell-script", script->priv->shell_script,
		      "for-each-file", script->priv->for_each_file,
		      "wait-command", script->priv->wait_command,
		      "accelerator", script->priv->accelerator,
		      NULL);

	return (GObject *) new_script;
}


static void
gth_script_finalize (GObject *base)
{
	GthScript *self;

	self = GTH_SCRIPT (base);
	g_free (self->priv->id);
	g_free (self->priv->display_name);
	g_free (self->priv->command);
	g_free (self->priv->accelerator);
	g_free (self->priv->detailed_action);

	G_OBJECT_CLASS (gth_script_parent_class)->finalize (base);
}


static char *
detailed_action_from_id (char *id)
{
	GVariant *param;
	char     *detailed_action;

	param = g_variant_new_string (id);
	detailed_action = g_action_print_detailed_name ("exec-script", param);

	g_variant_unref (param);

	return detailed_action;
}


static void
gth_script_set_property (GObject      *object,
			 guint         property_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	GthScript *self;

	self = GTH_SCRIPT (object);

	switch (property_id) {
	case PROP_ID:
		g_free (self->priv->id);
		self->priv->id = g_value_dup_string (value);
		if (self->priv->id == NULL)
			self->priv->id = g_strdup ("");
		g_free (self->priv->detailed_action);
		self->priv->detailed_action = detailed_action_from_id (self->priv->id);
		break;
	case PROP_DISPLAY_NAME:
		g_free (self->priv->display_name);
		self->priv->display_name = g_value_dup_string (value);
		if (self->priv->display_name == NULL)
			self->priv->display_name = g_strdup ("");
		break;
	case PROP_COMMAND:
		g_free (self->priv->command);
		self->priv->command = g_value_dup_string (value);
		if (self->priv->command == NULL)
			self->priv->command = g_strdup ("");
		break;
	case PROP_VISIBLE:
		self->priv->visible = g_value_get_boolean (value);
		break;
	case PROP_SHELL_SCRIPT:
		self->priv->shell_script = g_value_get_boolean (value);
		break;
	case PROP_FOR_EACH_FILE:
		self->priv->for_each_file = g_value_get_boolean (value);
		break;
	case PROP_WAIT_COMMAND:
		self->priv->wait_command = g_value_get_boolean (value);
		break;
	case PROP_ACCELERATOR:
		g_free (self->priv->accelerator);
		self->priv->accelerator = g_value_dup_string (value);
		break;
	default:
		break;
	}
}


static void
gth_script_get_property (GObject    *object,
			 guint       property_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	GthScript *self;

	self = GTH_SCRIPT (object);

	switch (property_id) {
	case PROP_ID:
		g_value_set_string (value, self->priv->id);
		break;
	case PROP_DISPLAY_NAME:
		g_value_set_string (value, self->priv->display_name);
		break;
	case PROP_COMMAND:
		g_value_set_string (value, self->priv->command);
		break;
	case PROP_VISIBLE:
		g_value_set_boolean (value, self->priv->visible);
		break;
	case PROP_SHELL_SCRIPT:
		g_value_set_boolean (value, self->priv->shell_script);
		break;
	case PROP_FOR_EACH_FILE:
		g_value_set_boolean (value, self->priv->for_each_file);
		break;
	case PROP_WAIT_COMMAND:
		g_value_set_boolean (value, self->priv->wait_command);
		break;
	case PROP_ACCELERATOR:
		g_value_set_string (value, self->priv->accelerator);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_script_class_init (GthScriptClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->set_property = gth_script_set_property;
	object_class->get_property = gth_script_get_property;
	object_class->finalize = gth_script_finalize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_ID,
					 g_param_spec_string ("id",
							      "ID",
							      "The object id",
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_DISPLAY_NAME,
					 g_param_spec_string ("display-name",
							      "Display name",
							      "The user visible name",
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_COMMAND,
					 g_param_spec_string ("command",
							      "Command",
							      "The command to execute",
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_VISIBLE,
					 g_param_spec_boolean ("visible",
							       "Visible",
							       "Whether this script should be visible in the script list",
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_SHELL_SCRIPT,
					 g_param_spec_boolean ("shell-script",
							       "Shell Script",
							       "Whether to execute the command inside a terminal (with sh)",
							       TRUE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_FOR_EACH_FILE,
					 g_param_spec_boolean ("for-each-file",
							       "Each File",
							       "Whether to execute the command on file at a time",
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_WAIT_COMMAND,
					 g_param_spec_boolean ("wait-command",
							       "Wait command",
							       "Whether to wait command to finish",
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_ACCELERATOR,
					 g_param_spec_string ("accelerator",
							      "Accelerator",
							      "The keyboard shortcut to activate the script",
							      "",
							      G_PARAM_READWRITE));
}


static void
gth_script_dom_domizable_interface_init (DomDomizableInterface *iface)
{
	iface->create_element = gth_script_real_create_element;
	iface->load_from_element = gth_script_real_load_from_element;
}


static void
gth_script_gth_duplicable_interface_init (GthDuplicableInterface *iface)
{
	iface->duplicate = gth_script_real_duplicate;
}


static void
gth_script_init (GthScript *self)
{
	self->priv = gth_script_get_instance_private (self);
	self->priv->id = NULL;
	self->priv->display_name = NULL;
	self->priv->command = NULL;
	self->priv->visible = FALSE;
	self->priv->shell_script = FALSE;
	self->priv->for_each_file = FALSE;
	self->priv->wait_command = FALSE;
	self->priv->accelerator = NULL;
	self->priv->detailed_action = NULL;
}


GthScript*
gth_script_new (void)
{
	GthScript *script;
	char      *id;

	id = _g_str_random (ID_LENGTH);
	script = (GthScript *) g_object_new (GTH_TYPE_SCRIPT, "id", id, NULL);
	g_free (id);

	return script;
}


const char *
gth_script_get_id (GthScript *script)
{
	return script->priv->id;
}


const char *
gth_script_get_display_name (GthScript *script)
{
	return script->priv->display_name;
}


const char *
gth_script_get_command (GthScript *script)
{
	return script->priv->command;
}


const char *
gth_script_get_detailed_action (GthScript *self)
{
	return self->priv->detailed_action;
}


gboolean
gth_script_is_visible (GthScript *script)
{
	return script->priv->visible;
}


gboolean
gth_script_is_shell_script (GthScript  *script)
{
	return script->priv->shell_script;
}


gboolean
gth_script_for_each_file (GthScript *script)
{
	return script->priv->for_each_file;
}


gboolean
gth_script_wait_command (GthScript *script)
{
	return script->priv->wait_command;
}


/* -- gth_script_get_requested_attributes -- */


static gboolean
collect_attributes_cb (gunichar   parent_code,
		       gunichar   code,
		       char     **args,
		       gpointer   user_data)
{
	GString *result = user_data;

	if (code == GTH_SCRIPT_CODE_FILE_ATTRIBUTE) {
		if (result->str[0] != 0)
			g_string_append_c (result, ',');
		g_string_append (result, args[0]);
	}

	return FALSE;
}


char *
gth_script_get_requested_attributes (GthScript *script)
{
	GString *result;
	char    *attributes;

	result = g_string_new ("");
	_g_template_for_each_token (script->priv->command,
				    TEMPLATE_FLAGS_NO_ENUMERATOR,
				    collect_attributes_cb,
				    result);

	if (result->str[0] == 0) {
		attributes = NULL;
		g_string_free (result, TRUE);
	}
	else
		attributes = g_string_free (result, FALSE);

	return attributes;
}


/* -- gth_script_get_command_line_async -- */


typedef struct {
	GList  *file_list;
	GError *error;
	GList  *asked_values;
	GList  *last_asked_value;
} EvalData;


typedef struct {
	EvalData        eval_data;
	GtkWindow      *parent;
	GthScript      *script;
	GtkBuilder     *builder;
	GthThumbLoader *thumb_loader;
	GtkCallback     dialog_callback;
	gpointer        user_data;
} CommandLineData;


typedef char * (*GetFileDataValueFunc) (GthFileData *file_data);


typedef struct {
	int        n_param;
	char      *prompt;
	char      *default_value;
	char      *value;
	GtkWidget *entry;
} AskedValue;


static AskedValue *
asked_value_new (int n_param)
{
	AskedValue *asked_value;

	asked_value = g_new (AskedValue, 1);
	asked_value->n_param = n_param;
	asked_value->prompt = g_strdup (_("Enter a value:"));;
	asked_value->default_value = NULL;
	asked_value->value = NULL;
	asked_value->entry = NULL;

	return asked_value;
}


static void
asked_value_free (AskedValue *asked_value)
{
	g_free (asked_value->prompt);
	g_free (asked_value->default_value);
	g_free (asked_value->value);
	g_free (asked_value);
}


static void
command_line_data_free (CommandLineData *command_data)
{
	_g_object_unref (command_data->thumb_loader);
	_g_object_unref (command_data->builder);
	g_list_free_full (command_data->eval_data.asked_values, (GDestroyNotify) asked_value_free);
	_g_object_list_unref (command_data->eval_data.file_list);
	g_object_unref (command_data->script);
	g_free (command_data);
}


static void
_append_file_list (GString              *str,
		   GList                *file_list,
		   GetFileDataValueFunc  func,
		   gboolean              quote_value)
{
	GList *scan;

	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		char        *value;
		char        *quoted;

		value = func (file_data);

		quoted = quote_value ? g_shell_quote (value) : g_strdup (value);
		g_string_append (str, quoted);

		if (scan->next != NULL)
			g_string_append_c (str, ' ');

		g_free (quoted);
		g_free (value);
	}
}


static char *
get_uri_func (GthFileData *file_data)
{
	return g_file_get_uri (file_data->file);
}


static char *
get_filename_func (GthFileData *file_data)
{
	return g_file_get_path (file_data->file);
}


static char *
get_basename_func (GthFileData *file_data)
{
	return g_file_get_basename (file_data->file);
}


static char *
get_basename_wo_ext_func (GthFileData *file_data)
{
	char *basename;
	char *basename_wo_ext;

	basename = g_file_get_basename (file_data->file);
	basename_wo_ext = _g_path_remove_extension (basename);

	g_free (basename);

	return basename_wo_ext;
}


static char *
get_ext_func (GthFileData *file_data)
{
	char *path;
	char *ext;

	path = g_file_get_path (file_data->file);
	ext = g_strdup (_g_path_get_extension (path));

	g_free (path);

	return ext;
}


static char *
get_parent_func (GthFileData *file_data)
{
	GFile *parent;
	char  *path;

	parent = g_file_get_parent (file_data->file);
	path = g_file_get_path (parent);

	g_object_unref (parent);

	return path;
}


static char *
_get_timestamp (const char *format,
		gboolean    quote_value)
{
	GDateTime *now;
	char      *str;

	now = g_date_time_new_now_local ();
	str = g_date_time_format (now, (format != NULL) ? format : DEFAULT_STRFTIME_FORMAT);
	if (quote_value) {
		char *tmp = str;
		str = g_shell_quote (tmp);
		g_free (tmp);
	}
	g_date_time_unref (now);

	return str;
}


static void
_append_attribute_list (GString  *str,
			GList    *file_list,
			char     *attribute,
			gboolean  quote_value)
{
	gboolean  first_value;
	GList    *scan;

	if (attribute == NULL)
		return;

	first_value = TRUE;
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		char        *value;
		char        *quoted;

		value = gth_file_data_get_attribute_as_string (file_data, attribute);
		if (value == NULL)
			continue;

		if (! first_value) {
			g_string_append_c (str, ' ');
			first_value = FALSE;
		}

		if (value != NULL) {
			char *tmp = _g_utf8_replace_pattern (value, "[\r\n]", " ");
			g_free (value);
			value = tmp;
		}
		quoted = quote_value ? g_shell_quote (value) : g_strdup (value);
		g_string_append (str, quoted);

		g_free (quoted);
		g_free (value);
	}
}


static gboolean
eval_template_cb (TemplateFlags   flags,
		  gunichar        parent_code,
		  gunichar        code,
		  char          **args,
		  GString        *result,
		  gpointer        user_data)
{
	EvalData *eval_data = user_data;
	gboolean  preview;
	gboolean  quote_values;
	gboolean  highlight;
	char     *text;

	if (parent_code == GTH_SCRIPT_CODE_TIMESTAMP) {
		/* strftime code, return the code itself. */
		_g_string_append_template_code (result, code, args);
		return FALSE;
	}

	preview = (flags & TEMPLATE_FLAGS_PREVIEW) != 0;
	quote_values = ((flags & TEMPLATE_FLAGS_PARTIAL) == 0) && (parent_code == 0);
	highlight = preview && (code != 0) && (parent_code == 0);
	text = NULL;

	if (highlight)
		g_string_append (result, "<span foreground=\"#4696f8\">");

	switch (code) {
	case GTH_SCRIPT_CODE_URI: /* File URI */
		_append_file_list (result, eval_data->file_list, get_uri_func, quote_values);
		break;

	case GTH_SCRIPT_CODE_PATH: /* File path */
		_append_file_list (result, eval_data->file_list, get_filename_func, quote_values);
		break;

	case GTH_SCRIPT_CODE_BASENAME: /* File basename */
		_append_file_list (result, eval_data->file_list, get_basename_func, quote_values);
		break;

	case GTH_SCRIPT_CODE_BASENAME_NO_EXTENSION: /* File basename, no extension */
		_append_file_list (result, eval_data->file_list, get_basename_wo_ext_func, quote_values);
		break;

	case GTH_SCRIPT_CODE_EXTENSION: /* File extension */
		_append_file_list (result, eval_data->file_list, get_ext_func, quote_values);
		break;

	case GTH_SCRIPT_CODE_PARENT_PATH: /* Parent path */
		_append_file_list (result, eval_data->file_list, get_parent_func, quote_values);
		break;

	case GTH_SCRIPT_CODE_TIMESTAMP: /* Timestamp */
		text = _get_timestamp (args[0], quote_values);
		break;

	case GTH_SCRIPT_CODE_ASK_VALUE: /* Ask value */
		if (preview) {
			if ((args[0] != NULL) && ! _g_utf8_all_spaces (args[1]))
				g_string_append (result, args[1]);
			else if (! _g_utf8_all_spaces (args[0]))
				_g_string_append_markup_escaped (result, "{ %s }", args[0]);
			else
				g_string_append_unichar (result, code);
		}
		else if (eval_data->last_asked_value != NULL) {
			AskedValue *asked_value;

			asked_value = eval_data->last_asked_value->data;
			text = quote_values ? g_shell_quote (asked_value->value) : g_strdup (asked_value->value);
			eval_data->last_asked_value = eval_data->last_asked_value->next;
		}
		break;

	case GTH_SCRIPT_CODE_FILE_ATTRIBUTE: /* File attribute */
		if (preview)
			g_string_append_printf (result, "{ %s }", args[0]);
		else
			_append_attribute_list (result, eval_data->file_list, args[0], quote_values);
		break;

	case GTH_SCRIPT_CODE_QUOTE: /* Quote text. */
		if (args[0] != NULL)
			text = g_shell_quote (args[0]);
		break;

	default:
		/* Code not recognized, return the code itself. */
		_g_string_append_template_code (result, code, args);
		break;
	}

	if (text != NULL) {
		g_string_append (result, text);
		g_free (text);
	}

	if (highlight)
		g_string_append (result, "</span>");

	return (eval_data->error != NULL);
}


static void
_gth_script_get_command_line (GTask *task)
{
	CommandLineData *command_data;
	char            *result;

	command_data = g_task_get_task_data (task);
	command_data->eval_data.last_asked_value = command_data->eval_data.asked_values;
	command_data->eval_data.error = NULL;

	result = _g_template_eval (command_data->script->priv->command,
				   TEMPLATE_FLAGS_NO_ENUMERATOR,
				   eval_template_cb,
				   &command_data->eval_data);

	if (command_data->eval_data.error != NULL) {
		g_free (result);
		g_task_return_error (task, command_data->eval_data.error);
	}
	else
		g_task_return_pointer (task, result, g_free);
}


static void
ask_values_dialog_response_cb (GtkDialog *dialog,
			       int        response_id,
			       gpointer   user_data)
{
	GTask           *task = user_data;
	CommandLineData *command_data;

	command_data = g_task_get_task_data (task);
	if (command_data->dialog_callback)
		command_data->dialog_callback (NULL, command_data->user_data);

	if (response_id != GTK_RESPONSE_OK) {
		GError *error = NULL;

		if (response_id == GTK_RESPONSE_NO)
			error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_SKIP_TO_NEXT_FILE, "");
		else
			error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, "");
		g_task_return_error (task, error);
	}
	else {
		GList *scan;

		for (scan = command_data->eval_data.asked_values; scan; scan = scan->next) {
			AskedValue *asked_value = scan->data;

			g_free (asked_value->value);
			asked_value->value = g_utf8_normalize (gtk_entry_get_text (GTK_ENTRY (asked_value->entry)), -1, G_NORMALIZE_NFC);
		}

		_gth_script_get_command_line (task);
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
thumb_loader_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	CommandLineData *command_data = user_data;
	cairo_surface_t *image;

	if (gth_thumb_loader_load_finish (GTH_THUMB_LOADER (source_object),
					  result,
					  &image,
					  NULL))
	{
		gtk_image_set_from_surface (GTK_IMAGE (_gtk_builder_get_widget (command_data->builder, "request_image")), image);
		cairo_surface_destroy (image);
	}

	g_object_unref (command_data->builder);
}


/* -- gth_script_get_command_line_async -- */


typedef struct {
	CommandLineData *command_data;
	int              n;
} CollectValuesData;


static gboolean
collect_asked_values_cb (gunichar   parent_code,
			 gunichar   code,
			 char     **args,
			 gpointer   user_data)
{
	CollectValuesData *collect_data = user_data;
	CommandLineData   *command_data = collect_data->command_data;
	AskedValue        *asked_value;

	if (code != GTH_SCRIPT_CODE_ASK_VALUE)
		return FALSE;

	asked_value = asked_value_new (collect_data->n++);
	asked_value->prompt = _g_utf8_strip (args[0]);
	asked_value->default_value = _g_utf8_strip (args[1]);
	command_data->eval_data.asked_values = g_list_prepend (command_data->eval_data.asked_values, asked_value);

	return FALSE;
}


void
gth_script_get_command_line_async (GthScript           *script,
				   GtkWindow           *parent,
				   GList               *file_list /* GthFileData */,
				   gboolean             can_skip,
				   GCancellable        *cancellable,
				   GtkCallback          dialog_callback,
				   GAsyncReadyCallback  callback,
				   gpointer             user_data)
{
	CommandLineData   *command_data;
	GTask             *task;
	CollectValuesData  collect_data;
	GthFileData       *file_data;
	GtkWidget         *dialog;

	command_data = g_new0 (CommandLineData, 1);
	command_data->script = g_object_ref (script);
	command_data->parent = parent;
	command_data->dialog_callback = dialog_callback;
	command_data->user_data = user_data;
	command_data->eval_data.file_list = _g_object_list_ref (file_list);
	command_data->eval_data.error = NULL;

	task = g_task_new (script, cancellable, callback, user_data);
	g_task_set_task_data (task, command_data, (GDestroyNotify) command_line_data_free);

	/* collect the values to ask to the user */

	collect_data.command_data = command_data;
	collect_data.n = 0;
	_g_template_for_each_token (script->priv->command,
				    TEMPLATE_FLAGS_NO_ENUMERATOR,
				    collect_asked_values_cb,
				    &collect_data);

	if (command_data->eval_data.asked_values == NULL) {
		/* No values to ask to the user. */
		_gth_script_get_command_line (task);
		return;
	}

	command_data->eval_data.asked_values = g_list_reverse (command_data->eval_data.asked_values);

	command_data->builder = gtk_builder_new_from_resource ("/org/gnome/gThumb/list_tools/data/ui/ask-values.ui");
	dialog = g_object_new (GTK_TYPE_DIALOG,
			       "title", "",
			       "transient-for", GTK_WINDOW (command_data->parent),
			       "modal", FALSE,
			       "destroy-with-parent", FALSE,
			       "use-header-bar", _gtk_settings_get_dialogs_use_header (),
			       "resizable", TRUE,
			       NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
			   _gtk_builder_get_widget (command_data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_GTK_LABEL_EXECUTE, GTK_RESPONSE_OK,
				! can_skip ? NULL : (gth_script_for_each_file (command_data->script) ? _("_Skip") : NULL), GTK_RESPONSE_NO,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (dialog),
					   GTK_RESPONSE_OK,
					   GTK_STYLE_CLASS_SUGGESTED_ACTION);

	gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (command_data->builder, "title_label")),
			    gth_script_get_display_name (command_data->script));

	file_data = (GthFileData *) command_data->eval_data.file_list->data;
	gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (command_data->builder, "filename_label")),
			    g_file_info_get_display_name (file_data->info));

	{
		GtkWidget *prompts = _gtk_builder_get_widget (command_data->builder, "prompts");
		GList     *scan;

		for (scan = command_data->eval_data.asked_values; scan; scan = scan->next) {
			AskedValue *asked_value = scan->data;
			GtkWidget  *label;
			GtkWidget  *entry;
			GtkWidget  *box;

			label = gtk_label_new (asked_value->prompt);
			gtk_label_set_xalign (GTK_LABEL (label), 0.0);

			entry = gtk_entry_new ();
			if (asked_value->default_value != NULL)
				gtk_entry_set_text (GTK_ENTRY (entry), asked_value->default_value);
			gtk_widget_set_size_request (entry, 300, -1);

			box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
			gtk_box_pack_start (GTK_BOX (box), label, TRUE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (box), entry, TRUE, FALSE, 0);
			gtk_widget_show_all (box);
			gtk_box_pack_start (GTK_BOX (prompts), box, FALSE, FALSE, 0);

			asked_value->entry = entry;
		}
	}

	g_object_ref (command_data->builder);
	command_data->thumb_loader = gth_thumb_loader_new (128);
	gth_thumb_loader_load (command_data->thumb_loader,
			       file_data,
			       NULL,
			       thumb_loader_ready_cb,
			       command_data);

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (ask_values_dialog_response_cb),
			  task);

	gtk_widget_show (dialog);

	if (command_data->dialog_callback)
		command_data->dialog_callback (dialog, command_data->user_data);
}


char *
gth_script_get_command_line_finish (GthScript       *script,
				    GAsyncResult    *result,
				    GError         **error)
{
	g_return_val_if_fail (g_task_is_valid (result, script), NULL);
	return g_task_propagate_pointer (G_TASK (result), error);
}


const char *
gth_script_get_accelerator (GthScript *self)
{
	g_return_val_if_fail (GTH_IS_SCRIPT (self), NULL);
	return self->priv->accelerator;
}


GthShortcut *
gth_script_create_shortcut (GthScript *self)
{
	GthShortcut *shortcut;

	shortcut = gth_shortcut_new ("exec-script", g_variant_new_string (gth_script_get_id (self)));
	shortcut->description = g_strdup (self->priv->display_name);
	shortcut->context = GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER;
	shortcut->category = GTH_SHORTCUT_CATEGORY_LIST_TOOLS;
	gth_shortcut_set_accelerator (shortcut, self->priv->accelerator);
	shortcut->default_accelerator = g_strdup ("");

	return shortcut;
}


/* -- gth_script_get_preview -- */


#define PREVIEW_URI "file:///home/user/images/filename.jpeg"


char *
gth_script_get_preview (const char    *template,
			TemplateFlags  flags)
{
	EvalData  preview_data;
	char     *result;

	preview_data.file_list = g_list_append (NULL,
						gth_file_data_new_for_uri (PREVIEW_URI, NULL));

	preview_data.error = NULL;
	preview_data.asked_values = NULL;
	preview_data.last_asked_value = NULL;

	result = _g_template_eval (template,
				   flags | TEMPLATE_FLAGS_NO_ENUMERATOR | TEMPLATE_FLAGS_PREVIEW,
				   eval_template_cb,
				   &preview_data);

	_g_object_list_unref (preview_data.file_list);

	return result;
}
