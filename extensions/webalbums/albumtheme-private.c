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
	if (cell->ref == 0) {
		if (cell->type == GTH_CELL_TYPE_VAR)
			g_free (cell->value.var);
		g_free (cell);
	}
}


/* GthExpr */


static int
default_get_var_value_func (const char *var_name,
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
gth_expr_push_expr (GthExpr *e, GthExpr *e2)
{
	int i;

	for (i = 0; i < e2->top; i++) {
		gth_cell_unref (e->data[e->top]);
		e->data[e->top] = gth_cell_ref (e2->data[i]);
		e->top++;
	}
}


void
gth_expr_push_op (GthExpr *e, GthOp op)
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
gth_expr_push_var (GthExpr *e, const char *name)
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
gth_expr_push_constant (GthExpr *e, int value)
{
	GthCell *cell;

	gth_cell_unref (e->data[e->top]);

	cell = gth_cell_new ();
	cell->type = GTH_CELL_TYPE_CONSTANT;
	cell->value.constant = value;
	e->data[e->top] = cell;

	e->top++;
}


void
gth_expr_pop (GthExpr *e)
{
	if (! gth_expr_is_empty (e))
		e->top--;
}


GthCell*
gth_expr_get_pos (GthExpr *e, int pos)
{
	if ((pos <= 0) || (pos > e->top))
		return NULL;
	return e->data[pos - 1];
}


GthCell*
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

	for (i = 0; i < gth_expr_get_top (e); i++) {
		GthCell *cell = gth_expr_get_pos (e, i + 1);

		switch (cell->type) {
		case GTH_CELL_TYPE_VAR:
			printf ("VAR: %s (%d)\n",
				cell->value.var,
				e->get_var_value_func (cell->value.var,
						       e->get_var_value_data));
			break;

		case GTH_CELL_TYPE_CONSTANT:
			printf ("NUM: %d\n", cell->value.constant);
			break;

		case GTH_CELL_TYPE_OP:
			printf ("OP: %s\n", op_name[cell->value.op]);
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

	for (i = 0; i < gth_expr_get_top (e); i++) {
		GthCell *cell = gth_expr_get_pos (e, i + 1);
		int      a, b, c;

		switch (cell->type) {
		case GTH_CELL_TYPE_VAR:
			gth_mem_push (mem,
				      e->get_var_value_func (cell->value.var,
							     e->get_var_value_data));
			break;

		case GTH_CELL_TYPE_CONSTANT:
			gth_mem_push (mem, cell->value.constant);
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


/* GthVar */


GthVar *
gth_var_new_constant (int value)
{
	GthVar *var;

	var = g_new0 (GthVar, 1);
	var->name = NULL;
	var->type = GTH_VAR_EXPR;
	var->value.expr = gth_expr_new ();
	gth_expr_push_constant (var->value.expr, value);

	return var;
}


GthVar *
gth_var_new_expression (const char *name,
			GthExpr    *e)
{
	GthVar *var;

	g_return_val_if_fail (name != NULL, NULL);

	var = g_new0 (GthVar, 1);
	var->type = GTH_VAR_EXPR;
	var->name = g_strdup (name);
	var->value.expr = gth_expr_ref (e);

	return var;
}


GthVar*
gth_var_new_string (const char *name,
                   const char *string)
{
	GthVar *var;

	g_return_val_if_fail (name != NULL, NULL);

	var = g_new0 (GthVar, 1);
	var->type = GTH_VAR_STRING;
	var->name = g_strdup (name);
	if (string != NULL)
		var->value.string = g_strdup (string);

	return var;
}


void
gth_var_free (GthVar *var)
{
	g_free (var->name);
	if (var->type == GTH_VAR_EXPR)
		gth_expr_unref (var->value.expr);
	if (var->type == GTH_VAR_STRING)
		g_free (var->value.string);
	g_free (var);
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
gth_condition_add_document  (GthCondition *cond,
			     GList        *document)
{
	if (cond->document != NULL)
		gth_parsed_doc_free (cond->document);
	cond->document = document;
}


/* GthTag */


GthTag *
gth_tag_new (GthTagType  type,
	     GList      *arg_list)
{
	GthTag *tag;

	tag = g_new0 (GthTag, 1);
	tag->type = type;
	tag->value.arg_list = arg_list;

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
	if (tag->type == GTH_TAG_HTML)
		g_free (tag->value.html);

	else if (tag->type == GTH_TAG_IF) {
		g_list_foreach (tag->value.cond_list,
				(GFunc) gth_condition_free,
				NULL);
		g_list_free (tag->value.cond_list);

	} else {
		g_list_foreach (tag->value.arg_list,
				(GFunc) gth_var_free,
				NULL);
		g_list_free (tag->value.arg_list);
	}

	if (tag->document != NULL)
		gth_parsed_doc_free (tag->document);

	g_free (tag);
}


void
gth_parsed_doc_free (GList *parsed_doc)
{
	if (parsed_doc != NULL) {
		g_list_foreach (parsed_doc, (GFunc) gth_tag_free, NULL);
		g_list_free (parsed_doc);
	}
}
