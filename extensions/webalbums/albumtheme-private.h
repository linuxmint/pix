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

#ifndef ALBUMTHEME_PRIVATE_H
#define ALBUMTHEME_PRIVATE_H

#include <glib.h>

/* A list of GthTag elements that describes a parsed .gthtml file */

extern GList *yy_parsed_doc;

/* GthMem: the memory stack used to evaluate an expression. */

typedef struct {
	int *data;
	int  top;
} GthMem;

GthMem*   gth_mem_new        (int     size);
void      gth_mem_free       (GthMem *mem);
void      gth_mem_set_empty  (GthMem *mem);
gboolean  gth_mem_is_empty   (GthMem *mem);
void      gth_mem_push       (GthMem *mem,
			      int     val);
int       gth_mem_pop        (GthMem *mem);
int       gth_mem_get_pos    (GthMem *mem,
			      int     pos);
int       gth_mem_get        (GthMem *mem);
int       gth_mem_get_top    (GthMem *mem);

/* GthCell: contains an element of the expression, therefore a GthExpr is a
 * series of GthCells. */

typedef enum {
	GTH_OP_ADD,
	GTH_OP_SUB,
	GTH_OP_MUL,
	GTH_OP_DIV,
	GTH_OP_NEG,
	GTH_OP_NOT,
	GTH_OP_AND,
	GTH_OP_OR,
	GTH_OP_CMP_EQ,
	GTH_OP_CMP_NE,
	GTH_OP_CMP_LT,
	GTH_OP_CMP_GT,
	GTH_OP_CMP_LE,
	GTH_OP_CMP_GE,
} GthOp;

typedef enum {
	GTH_CELL_TYPE_OP,
	GTH_CELL_TYPE_VAR,
	GTH_CELL_TYPE_STRING,
	GTH_CELL_TYPE_INTEGER
} GthCellType;

typedef struct {
	int         ref;
	GthCellType type;
	union {
		GthOp    op;
		char    *var;
		GString *string;
		int      integer;
	} value;
} GthCell;

GthCell*   gth_cell_new   (void);
GthCell*   gth_cell_ref   (GthCell *cell);
void       gth_cell_unref (GthCell *cell);

/* GthExpr */

typedef struct _GthExpr GthExpr;

typedef int (*GthGetVarValueFunc) (GthExpr    *expr,
				   int        *index,
				   const char *var_name,
				   gpointer    data);

struct _GthExpr {
	int                  ref;
	GthCell            **data;
	int                  top;
	GthGetVarValueFunc   get_var_value_func;
	gpointer             get_var_value_data;
};

GthExpr *  gth_expr_new                    (void);
GthExpr *  gth_expr_ref                    (GthExpr            *e);
void       gth_expr_unref                  (GthExpr            *e);
void       gth_expr_set_empty              (GthExpr            *e);
gboolean   gth_expr_is_empty               (GthExpr            *e);
void       gth_expr_push_expr              (GthExpr            *e,
					    GthExpr            *e2);
void       gth_expr_push_op                (GthExpr            *e,
					    GthOp               op);
void       gth_expr_push_var               (GthExpr            *e,
					    const char         *name);
void       gth_expr_push_string            (GthExpr            *e,
					    const char         *value);
void       gth_expr_push_integer           (GthExpr            *e,
					    int                 value);
void       gth_expr_pop                    (GthExpr            *e);
GthCell *  gth_expr_get_pos                (GthExpr            *e,
					    int                 pos);
GthCell *  gth_expr_get                    (GthExpr            *e);
int        gth_expr_get_top                (GthExpr            *e);
void       gth_expr_set_get_var_value_func (GthExpr            *e,
					    GthGetVarValueFunc  f,
					    gpointer            data);
void       gth_expr_print                  (GthExpr            *e);
int        gth_expr_eval                   (GthExpr            *e);
void       gth_expr_list_unref             (GList              *list);

/* GthAttribute */

typedef enum {
	GTH_ATTRIBUTE_EXPR,
	GTH_ATTRIBUTE_STRING
} GthAttributeType;

typedef struct {
	char             *name;
	GthAttributeType  type;
	union {
		GthExpr *expr;
		char    *string;
	} value;
} GthAttribute;

GthAttribute *  gth_attribute_new_expression  (const char   *name,
					       GthExpr      *expr);
GthAttribute *  gth_attribute_new_string      (const char   *name,
					       const char   *string);
void            gth_attribute_free            (GthAttribute *attribute);

/* GthCondition */

typedef struct {
	GthExpr *expr;
	GList   *document; /* GthTag list */
} GthCondition;

GthCondition * gth_condition_new           (GthExpr      *expr);
void           gth_condition_free          (GthCondition *cond);
void           gth_condition_add_document  (GthCondition *cond,
					    GList        *document);

/* GthLoop */

typedef enum {
	GTH_TAG_HEADER = 0,
	GTH_TAG_FOOTER,
	GTH_TAG_LANGUAGE,
	GTH_TAG_THEME_LINK,
	GTH_TAG_IMAGE,
	GTH_TAG_IMAGE_LINK,
	GTH_TAG_IMAGE_IDX,
	GTH_TAG_IMAGE_DIM,
	GTH_TAG_IMAGE_ATTRIBUTE,
	GTH_TAG_IMAGES,
	GTH_TAG_FILE_NAME,
	GTH_TAG_FILE_PATH,
	GTH_TAG_FILE_SIZE,
	GTH_TAG_PAGE_LINK,
	GTH_TAG_PAGE_IDX,
	GTH_TAG_PAGE_ROWS,
	GTH_TAG_PAGE_COLS,
	GTH_TAG_PAGES,
	GTH_TAG_THUMBNAILS,
	GTH_TAG_TIMESTAMP,
	GTH_TAG_TRANSLATE,
	GTH_TAG_HTML,
	GTH_TAG_SET_VAR,
	GTH_TAG_EVAL,
	GTH_TAG_IF,
	GTH_TAG_FOR_EACH_THUMBNAIL_CAPTION,
	GTH_TAG_FOR_EACH_IMAGE_CAPTION,
	GTH_TAG_FOR_EACH_IN_RANGE,
	GTH_TAG_ITEM_ATTRIBUTE,
	GTH_TAG_INVALID
} GthTagType;

typedef struct {
	GthTagType  type;
	GList      *document; /* GthTag list */
} GthLoop;

#define GTH_LOOP(x) ((GthLoop *)(x))

GthLoop *   gth_loop_new           (GthTagType  loop_type);
void        gth_loop_free          (GthLoop    *loop);
GthTagType  gth_loop_get_type      (GthLoop    *loop);
void        gth_loop_add_document  (GthLoop    *loop,
				    GList      *document);

/* GthRangeLopp */

typedef struct {
	GthLoop  parent;
	char    *iterator;
	GthExpr *first_value;
	GthExpr *last_value;
} GthRangeLoop;

#define GTH_RANGE_LOOP(x) ((GthRangeLoop *)(x))

GthLoop *  gth_range_loop_new       (void);
void       gth_range_loop_free      (GthRangeLoop *loop);
void       gth_range_loop_set_range (GthRangeLoop *loop,
				     const char   *iterator,
				     GthExpr      *first_value,
				     GthExpr      *last_value);

/* GthTag */

typedef struct {
	GthTagType type;
	union {
		GList        *attributes;  /* GthAttribute list */
		char         *html;        /* html */
		GList        *cond_list;   /* GthCondition list */
		GthLoop      *loop;        /* a loop tag */
		GthRangeLoop *range_loop;
	} value;
	GList *document; /* GthTag list */
} GthTag;

GthTag *     gth_tag_new                 (GthTagType  type,
				          GList      *attributes);
GthTag *     gth_tag_new_html            (const char *html);
GthTag *     gth_tag_new_condition       (GList      *cond_list);
GthTag *     gth_tag_new_loop            (GthLoop    *loop);
void         gth_tag_add_document        (GthTag     *tag,
				          GList      *document);
void         gth_tag_free                (GthTag     *tag);
GthTagType   gth_tag_get_type_from_name  (const char *tag_name);
const char * gth_tag_get_name_from_type  (GthTagType  tag_type);

/* Utils */

int      gth_albumtheme_yyparse      (void);
void     gth_parsed_doc_print_tree   (GList *document);
void     gth_parsed_doc_free         (GList *document);

#endif /* ALBUMTHEME_PRIVATE_H */
