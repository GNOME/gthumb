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

void	exec_script0		   	(GtkAction       *action, 
				    	 GthWindow       *window);
void	exec_script1                  	(GtkAction       *action,
                                    	 GthWindow       *window);
void	exec_script2                  	(GtkAction       *action,
                                    	 GthWindow       *window);
void	exec_script3                  	(GtkAction       *action,
                                    	 GthWindow       *window);
void	exec_script4                  	(GtkAction       *action,
                                    	 GthWindow       *window);
void	exec_script5                  	(GtkAction       *action,
                                    	 GthWindow       *window);
void	exec_script6                  	(GtkAction       *action,
                                    	 GthWindow       *window);
void	exec_script7                  	(GtkAction       *action,
                                    	 GthWindow       *window);
void	exec_script8                  	(GtkAction       *action,
                                    	 GthWindow       *window);
void	exec_script9                  	(GtkAction       *action,
                                    	 GthWindow       *window);

void	dlg_scripts 		   	(GthWindow       *window,
					 DoneFunc 	  done_func,
					 gpointer	  done_data);
guint	generate_script_menu	 	(GtkUIManager    *ui,
				    	 GtkActionGroup  *actions,
				     	 GthWindow       *window,
				    	 guint	    	  merge_id);

#endif /* DLG_SCRIPTS_H */
