%{
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
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <gio/gio.h>
#include "albumtheme-private.h"

int   gth_albumtheme_yylex   (void);
void  gth_albumtheme_yyerror (const char *fmt, ...);
void  gth_albumtheme_yyfatalerror (const char *msg);
int   gth_albumtheme_yywrap  (void);

#define YY_NO_UNPUT
#define YYERROR_VERBOSE 1

%}

%union {
	char         *text;
	int           ivalue;
	GthAttribute *attribute;
	GthTag       *tag;
	GthExpr      *expr;
	GList        *list;
	GthCondition *cond;
	GthLoop      *loop;
}

%nonassoc       IF ELSE ELSE_IF END SET_VAR PRINT
%token          END_TAG
%token <text>   VARIABLE ATTRIBUTE_NAME FUNCTION_NAME QUOTED_STRING
%token <ivalue> FOR_EACH
%token <ivalue> NUMBER
%token <ivalue> SET_VAR
%token <text>   HTML

%type <list>       document
%type <tag>        tag_command
%type <tag>        tag_print
%type <loop>       tag_loop
%type <cond>       tag_if
%type <cond>       tag_else_if
%type <cond>       opt_tag_else
%type <list>       opt_tag_else_if
%type <list>       attribute_list
%type <attribute>  attribute
%type <list>       expr_list
%type <expr>       expr

%left  <ivalue> BOOL_OP
%left  <ivalue> COMPARE
%left  RANGE IN
%left  '+' '-' '*' '/' '!' ','
%right UNARY_OP

%%

all		: document {
			yy_parsed_doc = $1;
			if (yy_parsed_doc == NULL)
				YYABORT;
			else
				YYACCEPT;
		}
		;

document	: HTML document {
			$$ = g_list_prepend ($2, gth_tag_new_html ($1));
			g_free ($1);
		}

		| tag_command document {
			$$ = g_list_prepend ($2, $1);
		}

		| tag_print document {
			$$ = g_list_prepend ($2, $1);
		}

		| tag_loop document tag_end document {
			GthTag *tag;

			gth_loop_add_document ($1, $2);
			tag = gth_tag_new_loop ($1);
			$$ = g_list_prepend ($4, tag);
		}

		| tag_if document opt_tag_else_if opt_tag_else tag_end document {
			GList  *cond_list;
			GthTag *tag;

			gth_condition_add_document ($1, $2);
			cond_list = g_list_prepend ($3, $1);
			if ($4 != NULL)
				cond_list = g_list_append (cond_list, $4);

			tag = gth_tag_new_condition (cond_list);
			$$ = g_list_prepend ($6, tag);
		}

		| /* empty */ {
			$$ = NULL;
		}

		| error document {
			/*if ($2 != NULL)
				gth_parsed_doc_free ($2);*/
			$$ = NULL;
		}
		;

tag_loop	: FOR_EACH VARIABLE END_TAG {
			if (g_str_equal ($2, "thumbnail_caption")) {
				$$ = gth_loop_new (GTH_TAG_FOR_EACH_THUMBNAIL_CAPTION);
			}
			else if (g_str_equal ($2, "image_caption")) {
				$$ = gth_loop_new (GTH_TAG_FOR_EACH_IMAGE_CAPTION);
			}
			else {
				yyerror ("Wrong iterator: '%s', expected 'thumbnail_caption' or 'image_caption'", $2);
				YYERROR;
			}
		}

		| FOR_EACH VARIABLE IN expr RANGE expr END_TAG {
			$$ = gth_range_loop_new ();
			gth_range_loop_set_range (GTH_RANGE_LOOP ($$), $2, $4, $6);

			g_free ($2);
			gth_expr_unref ($4);
			gth_expr_unref ($6);
		}
		;

tag_if		: IF expr END_TAG {
			$$ = gth_condition_new ($2);
		}

		| IF '"' expr '"' END_TAG {
			$$ = gth_condition_new ($3);
		}
		;

opt_tag_else_if	: tag_else_if document opt_tag_else_if {
			gth_condition_add_document ($1, $2);
			$$ = g_list_prepend ($3, $1);
		}

		| /* empty */ {
			$$ = NULL;
		}
		;

tag_else_if	: ELSE_IF expr END_TAG {
			$$ = gth_condition_new ($2);
		}

		| ELSE_IF '"' expr '"' END_TAG {
			$$ = gth_condition_new ($3);
		}
		;

opt_tag_else	: tag_else document {
			GthExpr      *else_expr;
			GthCondition *cond;

			else_expr = gth_expr_new ();
			gth_expr_push_integer (else_expr, 1);
			cond = gth_condition_new (else_expr);
			gth_condition_add_document (cond, $2);

			$$ = cond;
		}

		| /* empty */ {
			$$ = NULL;
		}
		;

tag_else	: ELSE END_TAG
		;

tag_end		: END END_TAG
		;

tag_command	: SET_VAR attribute_list END_TAG {
			$$ = gth_tag_new (GTH_TAG_SET_VAR, $2);
		}
		;

tag_print	: PRINT FUNCTION_NAME expr_list END_TAG {
			if (gth_tag_get_type_from_name ($2) == GTH_TAG_EVAL) {
				GthExpr *e;
				GList   *arg_list;

				if ($3 == NULL) {
					yyerror ("Missing argument for function 'eval', expected expression");
					YYERROR;
				}

				e = $3->data;
				arg_list = g_list_append (NULL, gth_attribute_new_expression ("expr", e));
				$$ = gth_tag_new (GTH_TAG_EVAL, arg_list);

				gth_expr_list_unref ($3);
			}
			else if (gth_tag_get_type_from_name ($2) == GTH_TAG_TRANSLATE) {
				GList *arg_list = NULL;
				GList *scan;

				for (scan = $3; scan; scan = scan->next) {
					GthExpr *e = scan->data;

					if (scan == $3) {
						GthCell *cell;

						cell = gth_expr_get (e);
						if (cell->type != GTH_CELL_TYPE_STRING) {
							yyerror ("Wrong argument type: %d, expected string", cell->type);
							YYERROR;
						}
						arg_list = g_list_append (arg_list, gth_attribute_new_string ("text", cell->value.string->str));

						continue;
					}

					arg_list = g_list_append (arg_list, gth_attribute_new_expression ("expr", e));
				}
				$$ = gth_tag_new (GTH_TAG_TRANSLATE, arg_list);

				gth_expr_list_unref ($3);
			}
			else {
				yyerror ("Wrong function: '%s', expected 'eval' or 'translate'", $2);
				YYERROR;
			}
		}

		| PRINT FUNCTION_NAME attribute_list END_TAG {
			GthTagType tag_type = gth_tag_get_type_from_name ($2);
			if (tag_type == GTH_TAG_INVALID) {
				yyerror ("Unrecognized function: %s", $2);
				YYERROR;
			}
			$$ = gth_tag_new (tag_type, $3);
		}
		;

attribute_list	: attribute attribute_list {
			$$ = g_list_prepend ($2, $1);
		}

		| /* empty */ {
			$$ = NULL;
		}
		;

attribute	: ATTRIBUTE_NAME '=' '"' expr '"' {
			$$ = gth_attribute_new_expression ($1, $4);
			g_free ($1);
		}

		| ATTRIBUTE_NAME '=' '\'' QUOTED_STRING '\'' {
			$$ = gth_attribute_new_string ($1, $4);
			g_free ($1);
			g_free ($4);
		}

		| ATTRIBUTE_NAME {
			GthExpr *e = gth_expr_new ();
			gth_expr_push_integer (e, 1);
			$$ = gth_attribute_new_expression ($1, e);
			g_free ($1);
		}
		;

expr_list	: expr ',' expr_list {
			$$ = g_list_prepend ($3, $1);
		}

		| expr {
			$$ = g_list_prepend (NULL, $1);
		}

		| /* empty */ {
			$$ = NULL;
		}
		;

expr		: '(' expr ')' {
			$$ = $2;
		}

		| expr COMPARE expr {
			GthExpr *e = gth_expr_new ();

			gth_expr_push_expr (e, $1);
			gth_expr_push_expr (e, $3);
			gth_expr_push_op (e, $2);

			gth_expr_unref ($1);
			gth_expr_unref ($3);

			$$ = e;
		}

		| expr '+' expr {
			GthExpr *e = gth_expr_new ();

			gth_expr_push_expr (e, $1);
			gth_expr_push_expr (e, $3);
			gth_expr_push_op (e, GTH_OP_ADD);

			gth_expr_unref ($1);
			gth_expr_unref ($3);

			$$ = e;
		}

		| expr '-' expr {
			GthExpr *e = gth_expr_new ();

			gth_expr_push_expr (e, $1);
			gth_expr_push_expr (e, $3);
			gth_expr_push_op (e, GTH_OP_SUB);

			gth_expr_unref ($1);
			gth_expr_unref ($3);

			$$ = e;
		}

		| expr '*' expr {
			GthExpr *e = gth_expr_new ();

			gth_expr_push_expr (e, $1);
			gth_expr_push_expr (e, $3);
			gth_expr_push_op (e, GTH_OP_MUL);

			gth_expr_unref ($1);
			gth_expr_unref ($3);

			$$ = e;
		}

		| expr '/' expr {
			GthExpr *e = gth_expr_new ();

			gth_expr_push_expr (e, $1);
			gth_expr_push_expr (e, $3);
			gth_expr_push_op (e, GTH_OP_DIV);

			gth_expr_unref ($1);
			gth_expr_unref ($3);

			$$ = e;
		}

		| expr BOOL_OP expr {
			GthExpr *e = gth_expr_new ();

			gth_expr_push_expr (e, $1);
			gth_expr_push_expr (e, $3);
			gth_expr_push_op (e, $2);

			gth_expr_unref ($1);
			gth_expr_unref ($3);

			$$ = e;
		}

		| '+' expr %prec UNARY_OP {
			$$ = $2;
		}

		| '-' expr %prec UNARY_OP {
			gth_expr_push_op ($2, GTH_OP_NEG);
			$$ = $2;
		}

		| '!' expr %prec UNARY_OP {
			gth_expr_push_op ($2, GTH_OP_NOT);
			$$ = $2;
		}

		| VARIABLE '(' expr_list ')' %prec UNARY_OP { /* function call */
			GthExpr *e = gth_expr_new ();
			gth_expr_push_var (e, $1);
			if ($3 != NULL) {
				GList *scan;

				for (scan = $3; scan; scan = scan->next) {
					GthExpr *arg = scan->data;
					gth_expr_push_expr (e, arg);
					gth_expr_unref (arg);
				}
				g_list_free ($3);
			}
			g_free ($1);
			$$ = e;
		}

		| VARIABLE {
			GthExpr *e = gth_expr_new ();
			gth_expr_push_var (e, $1);
			g_free ($1);
			$$ = e;
		}

		| '\'' QUOTED_STRING '\'' {
			GthExpr *e = gth_expr_new ();
			gth_expr_push_string (e, $2);
			g_free ($2);
			$$ = e;
		}

		| NUMBER {
			GthExpr *e = gth_expr_new ();
			gth_expr_push_integer (e, $1);
			$$ = e;
		}
		;

%%


int
gth_albumtheme_yywrap (void)
{
	return 1;
}


void
gth_albumtheme_yyerror (const char *fmt, ...)
{
	va_list ap;

	va_start (ap, fmt);
	vfprintf (stderr, fmt, ap);
	fprintf (stderr, "\n");
	va_end (ap);
}


void
gth_albumtheme_yyfatalerror (const char *msg)
{
	g_error ("%s", msg);
}

#include "albumtheme-lex.c"
