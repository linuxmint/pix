/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003, 2010 Free Software Foundation, Inc.
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

#include <stdio.h>
#include "albumtheme-private.h"

#define MAX_EXPR_SIZE 100
#define MEM_SIZE 1000


GList *yy_parsed_doc = NULL;


GthMem *
gth_mem_new (int size)
{
	GthMem *mem;

	mem = g_new0 (GthMem, 1);

	mem->data = g_new (int, size);
	gth_mem_set_empty (mem);

	return mem;
}


void
gth_mem_free (GthMem *mem)
{
	g_free (mem->data);
	g_free (mem);
}


void
gth_mem_set_empty (GthMem *mem)
{
	mem->top = 0;
}


gboolean
gth_mem_is_empty (GthMem *mem)
{
	return (mem->top == 0);
}


void
gth_mem_push (GthMem *mem,
	      int     val)
{
	mem->data[mem->top++] = val;
}


int
gth_mem_pop (GthMem *mem)
{
	if (! gth_mem_is_empty (mem)) {
		mem->top--;
		return mem->data[mem->top];
	}

	return 0;
}


int
gth_mem_get_pos (GthMem *mem,
		 int     pos)
{
	if ((pos <= 0) || (pos > mem->top))
		return 0;
	return mem->data[pos - 1];
}


int
gth_mem_get (GthMem *mem)
{
	return gth_mem_get_pos (mem, mem->top);
}


int
gth_mem_get_top (GthMem *mem)
{
	return mem->top;
}


/* GthCell */


GthCell *
gth_cell_new (void)
{
	GthCell *cell;

	cell = g_new0 (GthCell, 1);
	cell->ref = 1;

	return cell;
}


GthCell *
gth_cell_ref (GthCell *cell)
{
	cell->ref++;
	return cell;
}


void
gth_cell_unref (GthCell *cell)
{
	if (cell == NULL)
		return;

	cell->ref--;
	if (cell->ref > 0)
		return;

	if (cell->type == GTH_CELL_TYPE_VAR)
		g_free (cell->value.var);
	else if (cell->type == GTH_CELL_TYPE_STRING)
		g_string_free (cell->value.string, TRUE);
	g_free (cell);
}


/* GthExpr */


static int
default_get_var_value_func (GthExpr    *expr,
		   	    int        *index,
		   	    const char *var_name,
			    gpointer    data)
{
	return 0;
}


GthExpr *
gth_expr_new (void)
{
	GthExpr *e;

	e = g_new0 (GthExpr, 1);
	e->ref = 1;
	e->data = g_new0 (GthCell*, MAX_EXPR_SIZE);
	gth_expr_set_get_var_value_func (e, default_get_var_value_func, NULL);

	return e;
}


GthExpr *
gth_expr_ref (GthExpr *e)
{
	e->ref++;
	return e;
}


void
gth_expr_unref (GthExpr *e)
{
	if (e == NULL)
		return;

	e->ref--;

	if (e->ref == 0) {
		int i;

		for (i = 0; i < MAX_EXPR_SIZE; i++)
			gth_cell_unref (e->data[i]);
		g_free (e->data);
		g_free (e);
	}
}


void
gth_expr_set_empty (GthExpr *e)
{
	e->top = 0;
}


gboolean
gth_expr_is_empty (GthExpr *e)
{
	return (e->top == 0);
}


void
gth_expr_push_expr (GthExpr *e,
		    GthExpr *e2)
{
	int i;

	for (i = 0; i < e2->top; i++) {
		gth_cell_unref (e->data[e->top]);
		e->data[e->top] = gth_cell_ref (e2->data[i]);
		e->top++;
	}
}


void
gth_expr_push_op (GthExpr *e,
		  GthOp    op)
{
	GthCell *cell;

	gth_cell_unref (e->data[e->top]);

	cell = gth_cell_new ();
	cell->type = GTH_CELL_TYPE_OP;
	cell->value.op = op;
	e->data[e->top] = cell;

	e->top++;
}


void
gth_expr_push_var (GthExpr    *e,
		   const char *name)
{
	GthCell *cell;

	gth_cell_unref (e->data[e->top]);

	cell = gth_cell_new ();
	cell->type = GTH_CELL_TYPE_VAR;
	cell->value.var = g_strdup (name);
	e->data[e->top] = cell;

	e->top++;
}


void
gth_expr_push_string (GthExpr    *e,
		      const char *value)
{
	GthCell *cell;

	gth_cell_unref (e->data[e->top]);

	cell = gth_cell_new ();
	cell->type = GTH_CELL_TYPE_STRING;
	cell->value.string = g_string_new (value);
	e->data[e->top] = cell;

	e->top++;
}


void
gth_expr_push_integer (GthExpr *e,
		       int      value)
{
	GthCell *cell;

	gth_cell_unref (e->data[e->top]);

	cell = gth_cell_new ();
	cell->type = GTH_CELL_TYPE_INTEGER;
	cell->value.integer = value;
	e->data[e->top] = cell;

	e->top++;
}


void
gth_expr_pop (GthExpr *e)
{
	if (! gth_expr_is_empty (e))
		e->top--;
}


GthCell *
gth_expr_get_pos (GthExpr *e, int pos)
{
	if ((pos <= 0) || (pos > e->top))
		return NULL;
	return e->data[pos - 1];
}


GthCell *
gth_expr_get (GthExpr *e)
{
	return gth_expr_get_pos (e, e->top);
}


int
gth_expr_get_top (GthExpr *e)
{
	return e->top;
}


void
gth_expr_set_get_var_value_func (GthExpr            *e,
				 GthGetVarValueFunc  f,
				 gpointer            data)
{
	e->get_var_value_func = f;
	e->get_var_value_data = data;
}


static char *op_name[] = {
	"ADD",
	"SUB",
	"MUL",
	"DIV",
	"NEG",
	"NOT",
	"AND",
	"OR",
	"CMP_EQ",
	"CMP_NE",
	"CMP_LT",
	"CMP_GT",
	"CMP_LE",
	"CMP_GE"
};


void
gth_expr_print (GthExpr *e)
{
	int i;

	for (i = 1; i <= gth_expr_get_top (e); i++) {
		GthCell *cell = gth_expr_get_pos (e, i);

		switch (cell->type) {
		case GTH_CELL_TYPE_VAR:
			/*g_print ("VAR: %s (%d)\n",
				 cell->value.var,
				 e->get_var_value_func (e,
						        &i,
						        cell->value.var,
						        e->get_var_value_data));*/
			g_print ("(%d) VAR: %s\n", i, cell->value.var);
			break;

		case GTH_CELL_TYPE_STRING:
			g_print ("(%d) STRING: %s\n", i, cell->value.string->str);
			break;

		case GTH_CELL_TYPE_INTEGER:
			printf ("(%d) NUM: %d\n", i, cell->value.integer);
			break;

		case GTH_CELL_TYPE_OP:
			printf ("(%d) OP: %s\n", i, op_name[cell->value.op]);
			break;
		}
	}
}


int
gth_expr_eval (GthExpr *e)
{
	GthMem *mem;
	int     retval = 0;
	int     i;

	mem = gth_mem_new (MEM_SIZE);

	for (i = 1; i <= gth_expr_get_top (e); i++) {
		GthCell *cell = gth_expr_get_pos (e, i);
		int      a, b, c;

		switch (cell->type) {
		case GTH_CELL_TYPE_VAR:
			gth_mem_push (mem,
				      e->get_var_value_func (e,
						      	     &i,
						      	     cell->value.var,
							     e->get_var_value_data));
			break;

		case GTH_CELL_TYPE_STRING:
			/* only used as argument for a function */
			break;

		case GTH_CELL_TYPE_INTEGER:
			gth_mem_push (mem, cell->value.integer);
			break;

		case GTH_CELL_TYPE_OP:
			switch (cell->value.op) {
			case GTH_OP_NEG:
				a = gth_mem_pop (mem);
				gth_mem_push (mem, -a);
				break;

			case GTH_OP_NOT:
				a = gth_mem_pop (mem);
				gth_mem_push (mem, (a == 0) ? 1 : 0);
				break;

			case GTH_OP_ADD:
				b = gth_mem_pop (mem);
				a = gth_mem_pop (mem);
				c = a + b;
				gth_mem_push (mem, c);
				break;

			case GTH_OP_SUB:
				b = gth_mem_pop (mem);
				a = gth_mem_pop (mem);
				c = a - b;
				gth_mem_push (mem, c);
				break;

			case GTH_OP_MUL:
				b = gth_mem_pop (mem);
				a = gth_mem_pop (mem);
				c = a * b;
				gth_mem_push (mem, c);
				break;

			case GTH_OP_DIV:
				b = gth_mem_pop (mem);
				a = gth_mem_pop (mem);
				c = a / b;
				gth_mem_push (mem, c);
				break;

			case GTH_OP_AND:
				b = gth_mem_pop (mem);
				a = gth_mem_pop (mem);
				c = (a != 0) && (b != 0);
				gth_mem_push (mem, c);
				break;

			case GTH_OP_OR:
				b = gth_mem_pop (mem);
				a = gth_mem_pop (mem);
				c = (a != 0) || (b != 0);
				gth_mem_push (mem, c);
				break;

			case GTH_OP_CMP_EQ:
				b = gth_mem_pop (mem);
				a = gth_mem_pop (mem);
				c = (a == b);
				gth_mem_push (mem, c);
				break;

			case GTH_OP_CMP_NE:
				b = gth_mem_pop (mem);
				a = gth_mem_pop (mem);
				c = (a != b);
				gth_mem_push (mem, c);
				break;

			case GTH_OP_CMP_LT:
				b = gth_mem_pop (mem);
				a = gth_mem_pop (mem);
				c = (a < b);
				gth_mem_push (mem, c);
				break;

			case GTH_OP_CMP_GT:
				b = gth_mem_pop (mem);
				a = gth_mem_pop (mem);
				c = (a > b);
				gth_mem_push (mem, c);
				break;

			case GTH_OP_CMP_LE:
				b = gth_mem_pop (mem);
				a = gth_mem_pop (mem);
				c = (a <= b);
				gth_mem_push (mem, c);
				break;

			case GTH_OP_CMP_GE:
				b = gth_mem_pop (mem);
				a = gth_mem_pop (mem);
				c = (a >= b);
				gth_mem_push (mem, c);
				break;
			}
			break;
		}
	}

	retval = gth_mem_get (mem);

	gth_mem_free (mem);

	return retval;
}


void
gth_expr_list_unref (GList *list)
{
	GList *scan;

	for (scan = list; scan; scan = scan->next)
		gth_expr_unref ((GthExpr *) scan->data);
	g_list_free (list);
}


/* GthAttribute */


GthAttribute *
gth_attribute_new_expression (const char *name,
			      GthExpr    *expr)
{
	GthAttribute *attribute;

	g_return_val_if_fail (name != NULL, NULL);

	attribute = g_new0 (GthAttribute, 1);
	attribute->type = GTH_ATTRIBUTE_EXPR;
	attribute->name = g_strdup (name);
	attribute->value.expr = gth_expr_ref (expr);

	return attribute;
}


GthAttribute*
gth_attribute_new_string (const char *name,
			  const char *string)
{
	GthAttribute *attribute;

	g_return_val_if_fail (name != NULL, NULL);

	attribute = g_new0 (GthAttribute, 1);
	attribute->type = GTH_ATTRIBUTE_STRING;
	attribute->name = g_strdup (name);
	if (string != NULL)
		attribute->value.string = g_strdup (string);

	return attribute;
}


void
gth_attribute_free (GthAttribute *attribute)
{
	g_free (attribute->name);
	switch (attribute->type) {
	case GTH_ATTRIBUTE_EXPR:
		gth_expr_unref (attribute->value.expr);
		break;
	case GTH_ATTRIBUTE_STRING:
		g_free (attribute->value.string);
		break;
	}
	g_free (attribute);
}


/* GthCondition */


GthCondition *
gth_condition_new (GthExpr *expr)
{
	GthCondition *cond;

	cond = g_new0 (GthCondition, 1);
	cond->expr = gth_expr_ref (expr);

	return cond;
}


void
gth_condition_free (GthCondition *cond)
{
	if (cond == NULL)
		return;
	gth_expr_unref (cond->expr);
	gth_parsed_doc_free (cond->document);
	g_free (cond);
}


void
gth_condition_add_document (GthCondition *cond,
			    GList        *document)
{
	if (cond->document != NULL)
		gth_parsed_doc_free (cond->document);
	cond->document = document;
}


/* GthLoop */


GthLoop *
gth_loop_new (GthTagType  loop_type)
{
	GthLoop *loop;

	loop = g_new0 (GthLoop, 1);
	loop->type = loop_type;

	return loop;
}


void
gth_loop_free (GthLoop *loop)
{
	if (loop == NULL)
		return;
	gth_parsed_doc_free (loop->document);
	g_free (loop);
}


GthTagType
gth_loop_get_type (GthLoop *loop)
{
	return loop->type;
}


void
gth_loop_add_document (GthLoop *loop,
		       GList   *document)
{
	if (loop->document != NULL)
		gth_parsed_doc_free (loop->document);
	loop->document = document;
}


/* GthRangeLoop */


GthLoop *
gth_range_loop_new (void)
{
	GthLoop *loop;

	loop = (GthLoop *) g_new0 (GthRangeLoop, 1);
	loop->type = GTH_TAG_FOR_EACH_IN_RANGE;

	return loop;
}


void
gth_range_loop_free (GthRangeLoop *loop)
{
	g_free (loop->iterator);
	gth_expr_unref (loop->first_value);
	gth_expr_unref (loop->last_value);
	gth_loop_free (GTH_LOOP (loop));
}


void
gth_range_loop_set_range (GthRangeLoop *loop,
			  const char   *iterator,
			  GthExpr      *first_value,
			  GthExpr      *last_value)
{
	loop->iterator = g_strdup (iterator);
	loop->first_value = gth_expr_ref (first_value);
	loop->last_value = gth_expr_ref (last_value);
}


/* GthTag */


GthTag *
gth_tag_new (GthTagType  type,
	     GList      *attributes)
{
	GthTag *tag;

	tag = g_new0 (GthTag, 1);
	tag->type = type;
	tag->value.attributes = attributes;

	return tag;
}


GthTag *
gth_tag_new_html (const char *html)
{
	GthTag *tag;

	tag = g_new0 (GthTag, 1);
	tag->type = GTH_TAG_HTML;
	tag->value.html = g_strdup (html);

	return tag;
}


GthTag *
gth_tag_new_condition (GList *cond_list)
{
	GthTag *tag;

	tag = g_new0 (GthTag, 1);
	tag->type = GTH_TAG_IF;
	tag->value.cond_list = cond_list;

	return tag;
}


GthTag *
gth_tag_new_loop (GthLoop *loop)
{
	GthTag *tag;

	tag = g_new0 (GthTag, 1);
	tag->type = loop->type;
	if (loop->type == GTH_TAG_FOR_EACH_IN_RANGE)
		tag->value.range_loop = (GthRangeLoop *) loop;
	else
		tag->value.loop = loop;

	return tag;
}


void
gth_tag_add_document (GthTag *tag,
		      GList  *document)
{
	if (tag->document != NULL)
		gth_parsed_doc_free (tag->document);
	tag->document = document;
}


void
gth_tag_free (GthTag *tag)
{
	if (tag->type == GTH_TAG_HTML) {
		g_free (tag->value.html);
	}
	else if (tag->type == GTH_TAG_IF) {
		g_list_foreach (tag->value.cond_list,
				(GFunc) gth_condition_free,
				NULL);
		g_list_free (tag->value.cond_list);
	}
	else if ((tag->type == GTH_TAG_FOR_EACH_THUMBNAIL_CAPTION)
		 || (tag->type == GTH_TAG_FOR_EACH_IMAGE_CAPTION))
	{
		gth_loop_free (tag->value.loop);
	}
	else if (tag->type == GTH_TAG_FOR_EACH_IN_RANGE) {
		gth_range_loop_free (GTH_RANGE_LOOP (tag->value.range_loop));
	}
	else {
		g_list_foreach (tag->value.attributes,
				(GFunc) gth_attribute_free,
				NULL);
		g_list_free (tag->value.attributes);
	}

	if (tag->document != NULL)
		gth_parsed_doc_free (tag->document);

	g_free (tag);
}


GthTagType
gth_tag_get_type_from_name (const char *tag_name)
{
	if (tag_name == NULL)
		return GTH_TAG_INVALID;

	if (g_str_equal (tag_name, "header"))
		return GTH_TAG_HEADER;
	else if (g_str_equal (tag_name, "footer"))
		return GTH_TAG_FOOTER;
	else if (g_str_equal (tag_name, "language"))
		return GTH_TAG_LANGUAGE;
	else if (g_str_equal (tag_name, "theme_link"))
		return GTH_TAG_THEME_LINK;
	else if (g_str_equal (tag_name, "image"))
		return GTH_TAG_IMAGE;
	else if (g_str_equal (tag_name, "image_link"))
		return GTH_TAG_IMAGE_LINK;
	else if (g_str_equal (tag_name, "image_idx"))
		return GTH_TAG_IMAGE_IDX;
	else if (g_str_equal (tag_name, "image_dim"))
		return GTH_TAG_IMAGE_DIM;
	else if (g_str_equal (tag_name, "image_attribute"))
		return GTH_TAG_IMAGE_ATTRIBUTE;
	else if (g_str_equal (tag_name, "images"))
		return GTH_TAG_IMAGES;
	else if (g_str_equal (tag_name, "file_name"))
		return GTH_TAG_FILE_NAME;
	else if (g_str_equal (tag_name, "file_path"))
		return GTH_TAG_FILE_PATH;
	else if (g_str_equal (tag_name, "file_size"))
		return GTH_TAG_FILE_SIZE;
	else if (g_str_equal (tag_name, "page_link"))
		return GTH_TAG_PAGE_LINK;
	else if (g_str_equal (tag_name, "page_idx"))
		return GTH_TAG_PAGE_IDX;
	else if (g_str_equal (tag_name, "page_link"))
		return GTH_TAG_PAGE_LINK;
	else if (g_str_equal (tag_name, "page_rows"))
		return GTH_TAG_PAGE_ROWS;
	else if (g_str_equal (tag_name, "page_cols"))
		return GTH_TAG_PAGE_COLS;
	else if (g_str_equal (tag_name, "pages"))
		return GTH_TAG_PAGES;
	else if (g_str_equal (tag_name, "thumbnails"))
		return GTH_TAG_THUMBNAILS;
	else if (g_str_equal (tag_name, "timestamp"))
		return GTH_TAG_TIMESTAMP;
	else if (g_str_equal (tag_name, "translate"))
		return GTH_TAG_TRANSLATE;
	else if (g_str_equal (tag_name, "html"))
		return GTH_TAG_HTML;
	else if (g_str_equal (tag_name, "set_var"))
		return GTH_TAG_SET_VAR;
	else if (g_str_equal (tag_name, "eval"))
		return GTH_TAG_EVAL;
	else if (g_str_equal (tag_name, "if"))
		return GTH_TAG_IF;
	else if (g_str_equal (tag_name, "for_each_thumbnail_caption"))
		return GTH_TAG_FOR_EACH_THUMBNAIL_CAPTION;
	else if (g_str_equal (tag_name, "for_each_image_caption"))
		return GTH_TAG_FOR_EACH_IMAGE_CAPTION;
	else if (g_str_equal (tag_name, "for_each_in_range"))
		return GTH_TAG_FOR_EACH_IN_RANGE;
	else if (g_str_equal (tag_name, "item_attribute"))
		return GTH_TAG_ITEM_ATTRIBUTE;

	return GTH_TAG_INVALID;
}


const char *
gth_tag_get_name_from_type (GthTagType tag_type)
{
	static char *tag_name[] = {
		"header",
		"footer",
		"language",
		"theme_link",
		"image",
		"image_link",
		"image_idx",
		"image_dim",
		"images",
		"filename",
		"filepath",
		"filesize",
		"page_link",
		"page_idx",
		"page_rows",
		"page_cols",
		"pages",
		"thumbnails",
		"timestamp",
		"text",
		"html",
		"set_var",
		"eval",
		"if",
		"for_each_thumbnail_caption",
		"for_each_image_caption",
		"for_each_in_range",
		"item_attribute"
	};

	return tag_name[tag_type];
}


void
gth_parsed_doc_print_tree (GList *document)
{
	GList *scan;

	for (scan = document; scan; scan = scan->next) {
		GthTag *tag = scan->data;

		g_print ("<%s>\n", gth_tag_get_name_from_type (tag->type));

		if ((tag->type != GTH_TAG_HTML) && (tag->type != GTH_TAG_IF)) {
			GList *scan_arg;

			for (scan_arg = tag->value.attributes; scan_arg; scan_arg = scan_arg->next) {
				GthAttribute *attribute = scan_arg->data;

				g_print ("  %s = ", attribute->name);
				if (attribute->type == GTH_ATTRIBUTE_STRING)
					g_print ("%s\n", attribute->value.string);
				else
					gth_expr_print (attribute->value.expr);
			}
		}
	}
	g_print (".\n\n");
}


void
gth_parsed_doc_free (GList *document)
{
	if (document != NULL) {
		g_list_foreach (document, (GFunc) gth_tag_free, NULL);
		g_list_free (document);
	}
}
