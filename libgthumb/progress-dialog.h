/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001,2003 The Free Software Foundation, Inc.
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

#ifndef PROGRESS_DIALOG_H
#define PROGRESS_DIALOG_H


#include "typedefs.h"


typedef struct _ProgressDialog ProgressDialog;


ProgressDialog *  progress_dialog_new           (GtkWindow      *parent);

void              progress_dialog_destroy       (ProgressDialog *pd);

void              progress_dialog_show          (ProgressDialog *pd);

void              progress_dialog_hide          (ProgressDialog *pd);

void              progress_dialog_set_parent    (ProgressDialog *pd,
						 GtkWindow      *parent);

void              progress_dialog_set_progress  (ProgressDialog *pd,
						 double          fraction);

void              progress_dialog_set_info      (ProgressDialog *pd,
						 const char     *info);

void              progress_dialog_set_cancel_func (ProgressDialog *pd,
						   DoneFunc        done_func,
						   gpointer        done_data);


#endif /* PROGRESS_DIALOG_H */
