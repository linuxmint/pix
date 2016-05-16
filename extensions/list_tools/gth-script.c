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


static void gth_script_dom_domizable_interface_init (DomDomizableInterface *iface);
static void gth_script_gth_duplicable_interface_init (GthDuplicableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthScript,
			 gth_script,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
					        gth_script_dom_domizable_interface_init)
		         G_IMPLEMENT_INTERFACE (GTH_TYPE_DUPLICABLE,
		        		        gth_script_gth_duplicable_interface_init))


enum {
        PROP_0,
        PROP_ID,
        PROP_DISPLAY_NAME,
        PROP_COMMAND,
        PROP_VISIBLE,
        PROP_SHELL_SCRIPT,
        PROP_FOR_EACH_FILE,
        PROP_WAIT_COMMAND,
        PROP_SHORTCUT
};


struct _GthScriptPrivate {
	char     *id;
	char     *display_name;
	char     *command;
	gboolean  visible;
	gboolean  shell_script;
	gboolean  for_each_file;
	gboolean  wait_command;
	guint     shortcut;
};


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
					       "shortcut", gdk_keyval_name (self->priv->shortcut),
					       NULL);
	if (! self->priv->visible)
		dom_element_set_attribute (element, "display", "none");

	return element;
}


static guint
_gdk_keyval_from_name (const gchar *keyval_name)
{
	if (keyval_name != NULL)
		return gdk_keyval_from_name (keyval_name);
	else
		return GDK_KEY_VoidSymbol;
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
		      "shortcut", _gdk_keyval_from_name (dom_element_get_attribute (element, "shortcut")),
		      NULL);
}


GObject *
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
		      "shortcut", script->priv->shortcut,
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

	G_OBJECT_CLASS (gth_script_parent_class)->finalize (base);
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
	case PROP_SHORTCUT:
		self->priv->shortcut = g_value_get_uint (value);
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
	case PROP_SHORTCUT:
		g_value_set_uint (value, self->priv->wait_command);
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

	g_type_class_add_private (klass, sizeof (GthScriptPrivate));

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
					 PROP_SHORTCUT,
					 g_param_spec_uint ("shortcut",
							    "Shortcut",
							    "The keyboard shortcut to activate the script",
							    0,
							    G_MAXUINT,
							    GDK_KEY_VoidSymbol,
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
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_SCRIPT, GthScriptPrivate);
	self->priv->id = NULL;
	self->priv->display_name = NULL;
	self->priv->command = NULL;
}


GthScript*
gth_script_new (void)
{
	GthScript *script;
	char      *id;

	id = _g_rand_string (ID_LENGTH);
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


char *
gth_script_get_requested_attributes (GthScript *script)
{
	GRegex   *re;
	char    **a;
	int       i, n, j;
	char    **b;
	char     *attributes;

	re = g_regex_new ("%attr\\{([^}]+)\\}", 0, 0, NULL);
	a = g_regex_split (re, script->priv->command, 0);
	for (i = 0, n = 0; a[i] != NULL; i++)
		if ((i > 0) && (i % 2 == 0))
			n++;
	if (n == 0)
		return NULL;

	b = g_new (char *, n + 1);
	for (i = 1, j = 0; a[i] != NULL; i += 2, j++) {
		b[j] = g_strstrip (a[i]);
	}
	b[j] = NULL;
	attributes = g_strjoinv (",", b);

	g_free (b);
	g_strfreev (a);
	g_regex_unref (re);

	return attributes;
}


/* -- gth_script_get_command_line -- */


typedef struct {
	GtkWindow  *parent;
	GthScript  *script;
	GList      *file_list;
	GError    **error;
	gboolean    quote_values;
} ReplaceData;


typedef char * (*GetFileDataValueFunc) (GthFileData *file_data);


static char *
create_file_list (GList                *file_list,
		  GetFileDataValueFunc  func,
		  gboolean              quote_value)
{
	GString *s;
	GList   *scan;

	s = g_string_new ("");
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		char        *value;
		char        *quoted;

		value = func (file_data);
		if (quote_value)
			quoted = g_shell_quote (value);
		else
			quoted = g_strdup (value);

		g_string_append (s, quoted);
		if (scan->next != NULL)
			g_string_append (s, " ");

		g_free (quoted);
		g_free (value);
	}

	return g_string_free (s, FALSE);
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
	basename_wo_ext = _g_uri_remove_extension (basename);

	g_free (basename);

	return basename_wo_ext;
}


static char *
get_ext_func (GthFileData *file_data)
{
	char *path;
	char *ext;

	path = g_file_get_path (file_data->file);
	ext = g_strdup (_g_uri_get_file_extension (path));

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


static void
thumb_loader_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	GtkBuilder *builder = user_data;
	GdkPixbuf  *pixbuf;

	if (! gth_thumb_loader_load_finish (GTH_THUMB_LOADER (source_object),
		  	 	 	    result,
		  	 	 	    &pixbuf,
		  	 	 	    NULL))
	{
		return;
	}

	if (pixbuf != NULL) {
		gtk_image_set_from_pixbuf (GTK_IMAGE (_gtk_builder_get_widget (builder, "request_image")), pixbuf);
		g_object_unref (pixbuf);
	}

	g_object_unref (builder);
}


static char *
ask_value (ReplaceData  *replace_data,
	   char         *match,
	   GError      **error)
{
	GthFileData     *file_data;
	GRegex          *re;
	char           **a;
	int              len;
	char            *prompt;
	char            *default_value;
	GtkBuilder      *builder;
	GtkWidget       *dialog;
	GthThumbLoader  *thumb_loader;
	int              result;
	char            *value;

	file_data = (GthFileData *) replace_data->file_list->data;

	re = g_regex_new ("%ask(\\{([^}]+)\\}(\\{([^}]+)\\})?)?", 0, 0, NULL);
	a = g_regex_split (re, match, 0);
	len = g_strv_length (a);
	if (len >= 3)
		prompt = g_strstrip (a[2]);
	else
		prompt = _("Enter a value:");
	if (len >= 5)
		default_value = g_strstrip (a[4]);
	else
		default_value = "";

	builder = _gtk_builder_new_from_file ("ask-value.ui", "list_tools");
	dialog = _gtk_builder_get_widget (builder, "ask_value_dialog");
	gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (builder, "title_label")), gth_script_get_display_name (replace_data->script));
	gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (builder, "filename_label")), g_file_info_get_display_name (file_data->info));
	gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (builder, "request_label")), prompt);
	gtk_entry_set_text (GTK_ENTRY (_gtk_builder_get_widget (builder, "request_entry")), default_value);
	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_window_set_transient_for (GTK_WINDOW (dialog), replace_data->parent);
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	if (! gth_script_for_each_file (replace_data->script))
		gtk_widget_hide (_gtk_builder_get_widget (builder, "skip_button"));

	g_object_ref (builder);
	thumb_loader = gth_thumb_loader_new (128);
	gth_thumb_loader_load (thumb_loader,
			       file_data,
			       NULL,
			       thumb_loader_ready_cb,
			       builder);

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	if (result == 2) {
		value = g_utf8_normalize (gtk_entry_get_text (GTK_ENTRY (_gtk_builder_get_widget (builder, "request_entry"))), -1, G_NORMALIZE_NFC);
	}
	else {
		if (result == 3)
			*error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_SKIP_TO_NEXT_FILE, "");
		else
			*error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, "");
		value = NULL;
	}

	gtk_widget_destroy (dialog);

	g_object_unref (builder);
	g_strfreev (a);
	g_regex_unref (re);

	return value;
}


static char *
create_attribute_list (GList    *file_list,
		      char     *match,
		      gboolean  quote_value)
{
	GRegex    *re;
	char     **a;
	char      *attribute = NULL;
	gboolean   first_value = TRUE;
	GString   *s;
	GList     *scan;

	re = g_regex_new ("%attr\\{([^}]+)\\}", 0, 0, NULL);
	a = g_regex_split (re, match, 0);
	if (g_strv_length (a) >= 2)
		attribute = g_strstrip (a[1]);

	if (attribute == NULL) {
		g_strfreev (a);
		g_regex_unref (re);
		return NULL;
	}

	s = g_string_new ("");
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		char        *value;
		char        *quoted;

		value = gth_file_data_get_attribute_as_string (file_data, attribute);
		if (value == NULL)
			continue;

		if (value != NULL) {
			char *tmp_value;

			tmp_value = _g_utf8_replace (value, "[\r\n]", " ");
			g_free (value);
			value = tmp_value;
		}
		if (quote_value)
			quoted = g_shell_quote (value);
		else
			quoted = g_strdup (value);

		if (! first_value)
			g_string_append (s, " ");
		g_string_append (s, quoted);

		first_value = FALSE;

		g_free (quoted);
		g_free (value);
	}

	g_strfreev (a);
	g_regex_unref (re);

	return g_string_free (s, FALSE);
}


static gboolean
command_line_eval_cb (const GMatchInfo *info,
		      GString          *res,
		      gpointer          data)
{
	ReplaceData *replace_data = data;
	char        *r = NULL;
	char        *match;

	match = g_match_info_fetch (info, 0);
	if (strcmp (match, "%U") == 0)
		r = create_file_list (replace_data->file_list, get_uri_func, replace_data->quote_values);
	else if (strcmp (match, "%F") == 0)
		r = create_file_list (replace_data->file_list, get_filename_func, replace_data->quote_values);
	else if (strcmp (match, "%B") == 0)
		r = create_file_list (replace_data->file_list, get_basename_func, replace_data->quote_values);
	else if (strcmp (match, "%N") == 0)
		r = create_file_list (replace_data->file_list, get_basename_wo_ext_func, replace_data->quote_values);
	else if (strcmp (match, "%E") == 0)
		r = create_file_list (replace_data->file_list, get_ext_func, replace_data->quote_values);
	else if (strcmp (match, "%P") == 0)
		r = create_file_list (replace_data->file_list, get_parent_func, replace_data->quote_values);
	else if (strncmp (match, "%attr", 5) == 0) {
		r = create_attribute_list (replace_data->file_list, match, replace_data->quote_values);
		if (r == NULL)
			*replace_data->error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_FAILED, _("Malformed command"));
	}
	else if (strncmp (match, "%ask", 4) == 0) {
		r = ask_value (replace_data, match, replace_data->error);
		if ((r != NULL) && replace_data->quote_values) {
			char *q;

			q = g_shell_quote (r);
			g_free (r);
			r = q;
		}
	}

	if (r != NULL)
		g_string_append (res, r);

	g_free (r);
	g_free (match);

	return FALSE;
}


char *
gth_script_get_command_line (GthScript  *script,
			     GtkWindow  *parent,
			     GList      *file_list /* GthFileData */,
			     GError    **error)
{
	ReplaceData  *replace_data;
	GRegex       *qre;
	GRegex       *re;
	char        **a;
	GString      *command_line;
	int           i;
	char         *result;

	replace_data = g_new0 (ReplaceData, 1);
	replace_data->parent = parent;
	replace_data->script = script;
	replace_data->file_list = file_list;
	replace_data->error = error;

	re = g_regex_new ("%U|%F|%B|%N|%E|%P|%ask(\\{[^}]+\\}(\\{[^}]+\\})?)?|%attr\\{[^}]+\\}", 0, 0, NULL);

	replace_data->quote_values = FALSE;
	command_line = g_string_new ("");
	qre = g_regex_new ("%quote\\{([^}]+)\\}", 0, 0, NULL);
	a = g_regex_split (qre, script->priv->command, 0);
	for (i = 0; a[i] != NULL; i++) {
		if (i % 2 == 1) {
			char *sub_result;
			char *quoted;

			sub_result = g_regex_replace_eval (re, a[i], -1, 0, 0, command_line_eval_cb, replace_data, error);
			quoted = g_shell_quote (g_strstrip (sub_result));
			g_string_append (command_line, quoted);

			g_free (quoted);
			g_free (sub_result);
		}
		else
			g_string_append (command_line, a[i]);
	}

	replace_data->quote_values = TRUE;
	result = g_regex_replace_eval (re, command_line->str, -1, 0, 0, command_line_eval_cb, replace_data, error);

	g_free (replace_data);
	g_string_free (command_line, TRUE);
	g_regex_unref (qre);
	g_regex_unref (re);

	return result;
}


guint
gth_script_get_shortcut (GthScript *script)
{
	return script->priv->shortcut;
}
