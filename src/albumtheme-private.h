/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef ALBUMTHEME_PRIVATE_H
#define ALBUMTHEME_PRIVATE_H

#include <glib.h>

extern GList *yy_parsed_doc;

/* GthMem */

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

/* GthCell */

typedef enum {
	GTH_OP_ADD,
	GTH_OP_SUB,
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
	GTH_CELL_TYPE_CONSTANT
} GthCellType;

typedef struct {
	int         ref;
	GthCellType type;
	union {
		GthOp  op;
		char  *var;
		int    constant;
	} value;
} GthCell;

GthCell*   gth_cell_new   (void);

GthCell*   gth_cell_ref   (GthCell *cell);

void       gth_cell_unref (GthCell *cell);

/* GthExpr */

typedef int (*GthGetVarValueFunc) (const char *var_name, gpointer data);

typedef struct {
	int                  ref;
	GthCell            **data;
	int                  top;
	GthGetVarValueFunc   get_var_value_func;
	gpointer             get_var_value_data;
} GthExpr;

GthExpr*  gth_expr_new               (void);

GthExpr*  gth_expr_ref               (GthExpr *e);

void      gth_expr_unref             (GthExpr *e);

void      gth_expr_set_empty         (GthExpr *e);

gboolean  gth_expr_is_empty          (GthExpr *e);

void      gth_expr_push_expr         (GthExpr *e, GthExpr *e2);

void      gth_expr_push_op           (GthExpr *e, GthOp op);

void      gth_expr_push_var          (GthExpr *e, const char *name);

void      gth_expr_push_constant     (GthExpr *e, int value);

void      gth_expr_pop               (GthExpr *e);

GthCell*  gth_expr_get_pos           (GthExpr *e, int pos);

GthCell*  gth_expr_get               (GthExpr *e);

int       gth_expr_get_top           (GthExpr *e);

void      gth_expr_set_get_var_value_func (GthExpr *e,
					   GthGetVarValueFunc f,
					   gpointer data);

int       gth_expr_eval              (GthExpr *e);

/* GthVar */

typedef struct {
	char     *name;
	GthExpr  *expr;
} GthVar;

GthVar*  gth_var_new_constant   (int value);

GthVar*  gth_var_new_expression (const char *name, GthExpr *e);

void     gth_var_free           (GthVar *var);

/* GthCondition */

typedef struct {
	GthExpr *expr;
	GList   *parsed_doc;
} GthCondition;

GthCondition * gth_condition_new      (GthExpr      *expr);

void           gth_condition_free     (GthCondition *cond);

void           gth_condition_add_doc  (GthCondition *cond,
				       GList        *parsed_doc);

/* GthTag */

typedef enum {
	GTH_TAG_TITLE,
	GTH_TAG_IMAGE,
	GTH_TAG_IMAGE_LINK,
	GTH_TAG_IMAGE_IDX,
	GTH_TAG_IMAGE_DIM,
	GTH_TAG_IMAGES,
	GTH_TAG_FILENAME,
	GTH_TAG_FILEPATH,
	GTH_TAG_FILESIZE,
	GTH_TAG_COMMENT,
	GTH_TAG_PAGE_LINK,
	GTH_TAG_PAGE_IDX,
	GTH_TAG_PAGES,
	GTH_TAG_TABLE,
	GTH_TAG_DATE,
	GTH_TAG_TEXT,
	GTH_TAG_SET_VAR,
	GTH_TAG_IF,
	GTH_TAG_EXIF_EXPOSURE_TIME,
	GTH_TAG_EXIF_EXPOSURE_MODE,
	GTH_TAG_EXIF_FLASH,
	GTH_TAG_EXIF_SHUTTER_SPEED,
	GTH_TAG_EXIF_APERTURE_VALUE,
	GTH_TAG_EXIF_FOCAL_LENGTH,
	GTH_TAG_EXIF_DATE_TIME,
	GTH_TAG_EXIF_CAMERA_MODEL
} GthTagType;

typedef struct {
	GthTagType type;
	union {
		GList *arg_list;    /* GthVar list */ 
		char  *text;        /* text */
		GList *cond_list;   /* GthCondition list */
	} value;
} GthTag;

GthTag * gth_tag_new                 (GthTagType    type, 
				      GList        *arg_list);

GthTag * gth_tag_new_text            (const char   *text);

GthTag * gth_tag_new_condition       (GList        *cond_list);

void     gth_tag_free                (GthTag       *tag);

/**/

void     gth_parsed_doc_free         (GList *parsed_doc);

#endif /* ALBUMTHEME_PRIVATE_H */
