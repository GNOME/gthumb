/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 The Free Software Foundation, Inc.
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

#ifndef DLG_SCRIPTS_H
#define DLG_SCRIPTS_H

#include "gth-window.h"

typedef struct {
	/* The number of the script */
	unsigned int number;

	/* Associated GThumb window */
	GthWindow*   window;
} ScriptCallbackData;

void	exec_script	   	(GtkAction          *action, 
			    	 ScriptCallbackData *cb_data);

void	dlg_scripts 	   	(GthWindow          *window,
				 DoneFunc 	     done_func,
				 gpointer	     done_data);
void	generate_script_menu	(GtkUIManager       *ui,
			    	 GtkActionGroup     *actions,
			     	 GthWindow          *window);

#endif /* DLG_SCRIPTS_H */
