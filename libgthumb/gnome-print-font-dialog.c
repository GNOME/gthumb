/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gnome-print-font-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2001-2002 Paolo Maggi 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */

/*
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <gtk/gtkdialog.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>

#include <libgnome/gnome-i18n.h>

#include <gnome-print-font-dialog.h>

struct _GnomePrintFontDialog {
	GtkDialog dialog;

	GtkWidget *fontsel;
	GtkWidget *preview;
};

struct _GnomePrintFontDialogClass {
	GtkDialogClass parent_class;
};

static void gnome_print_font_dialog_class_init (GnomePrintFontDialogClass *klass);
static void gnome_print_font_dialog_init (GnomePrintFontDialog *fontseldiag);

static void gfsd_update_preview (GnomeFontSelection * fs, GnomeFont * font, GnomePrintFontDialog * gfsd);

static GtkDialogClass *gfsd_parent_class = NULL;

GType
gnome_print_font_dialog_get_type (void)
{
	static GType font_dialog_type = 0;

	if (!font_dialog_type)
    	{
      		static const GTypeInfo dialog_info =
      		{
			sizeof (GnomePrintFontDialogClass),
        		NULL,		/* base_init */
        		NULL,		/* base_finalize */
        		(GClassInitFunc) gnome_print_font_dialog_class_init,
        		NULL,           /* class_finalize */
        		NULL,           /* class_data */
        		sizeof (GnomePrintFontDialog),
        		0,              /* n_preallocs */
        		(GInstanceInitFunc) gnome_print_font_dialog_init
      		};

     		font_dialog_type = g_type_register_static (GTK_TYPE_DIALOG,
							   "GnomePrintFontDialog",
							   &dialog_info, 
							   0);
    	}

	return font_dialog_type;

}
static void
gfsd_modify_preview_phrase (GtkButton *button, GnomePrintFontDialog *fontseldiag)
{
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *vbox;

	vbox = gtk_vbox_new (FALSE, 6);
	entry = gtk_entry_new();

	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);

	label = gtk_label_new_with_mnemonic (_("_Insert a new preview phrase."));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT (label), "xalign", 0.0, "yalign", 0.5, NULL);
		
	dialog = gtk_dialog_new_with_buttons (_("Modify preview phrase..."),
					      GTK_WINDOW (fontseldiag),
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_OK,
					      NULL);

	gtk_window_set_default_size (GTK_WINDOW (dialog), 300, -1);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, TRUE, TRUE, 0);

	gtk_widget_show_all (vbox);

	gtk_widget_grab_focus (entry);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
	{
		if (strlen (gtk_entry_get_text (GTK_ENTRY (entry))) > 0)
			 gnome_font_preview_set_phrase (
					 GNOME_FONT_PREVIEW (fontseldiag->preview),
					 gtk_entry_get_text (GTK_ENTRY (entry)));
	}

	gtk_widget_destroy (dialog);
}
	

static void
gnome_print_font_dialog_class_init (GnomePrintFontDialogClass *klass)
{
	GtkObjectClass *object_class;
  
	object_class = (GtkObjectClass*) klass;
  
	gfsd_parent_class = gtk_type_class (GTK_TYPE_DIALOG);
}

static GtkWidget*
gnome_print_font_dialog_create_preview_frame (GnomePrintFontDialog *fontseldiag)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *bbox;
	GtkWidget *button;

	frame = gtk_frame_new (_("Preview"));
	vbox = gtk_vbox_new (FALSE, 6);
	bbox = gtk_hbutton_box_new ();
	
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

	fontseldiag->preview = gnome_font_preview_new ();
	gtk_box_pack_start (GTK_BOX (vbox), fontseldiag->preview, TRUE, TRUE, 0);

	
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox),
				   GTK_BUTTONBOX_END);  

	button = gtk_button_new_with_mnemonic (_("_Modify preview phrase..."));
	gtk_box_pack_end (GTK_BOX (bbox), button, FALSE, TRUE, 0);

	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (gfsd_modify_preview_phrase), fontseldiag);

	gtk_box_pack_end (GTK_BOX (vbox), bbox, FALSE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (frame), vbox);

	return frame;
}

static void
gnome_print_font_dialog_init (GnomePrintFontDialog *fontseldiag)
{
	GnomeFont *font;
	GtkWidget *vbox;
	GtkWidget *preview_frame;

	gtk_widget_set_size_request (GTK_WIDGET (fontseldiag), 450, 300);
	
	gtk_dialog_add_button (GTK_DIALOG (fontseldiag), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (fontseldiag), GTK_STOCK_OK, GTK_RESPONSE_OK);
	
	gtk_dialog_set_default_response (GTK_DIALOG (fontseldiag), GTK_RESPONSE_CANCEL);
	
	gtk_container_set_border_width (GTK_CONTAINER (fontseldiag), 4);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);

	fontseldiag->fontsel = gnome_font_selection_new ();
	preview_frame = gnome_print_font_dialog_create_preview_frame (fontseldiag);

	gtk_box_pack_start (GTK_BOX (vbox), fontseldiag->fontsel, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), preview_frame, FALSE, FALSE, 0);

	font = gnome_font_selection_get_font (GNOME_FONT_SELECTION (fontseldiag->fontsel));
	gnome_font_preview_set_font (GNOME_FONT_PREVIEW (fontseldiag->preview), font);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fontseldiag)->vbox), vbox, TRUE, TRUE, 0);

	g_signal_connect (G_OBJECT (fontseldiag->fontsel), "font_set",
			  G_CALLBACK (gfsd_update_preview), fontseldiag);

	gtk_widget_show_all (vbox);
}

GtkWidget*
gnome_print_font_dialog_new	(const gchar	  *title)
{
	GnomePrintFontDialog *fontseldiag;
  
	fontseldiag = g_object_new (GNOME_PRINT_TYPE_FONT_DIALOG, NULL);

	gtk_window_set_title (GTK_WINDOW (fontseldiag), title ? title : _("Font Selection"));

	return GTK_WIDGET (fontseldiag);
}

GtkWidget *
gnome_print_font_dialog_get_fontsel (GnomePrintFontDialog *gfsd)
{
	g_return_val_if_fail (gfsd != NULL, NULL);
	g_return_val_if_fail (GNOME_PRINT_IS_FONT_DIALOG (gfsd), NULL);

	return gfsd->fontsel;
}

GtkWidget *
gnome_print_font_dialog_get_preview (GnomePrintFontDialog *gfsd)
{
	g_return_val_if_fail (gfsd != NULL, NULL);
	g_return_val_if_fail (GNOME_PRINT_IS_FONT_DIALOG (gfsd), NULL);

	return gfsd->preview;
}

static void
gfsd_update_preview (GnomeFontSelection * fs, GnomeFont * font, GnomePrintFontDialog * gfsd)
{
	gnome_font_preview_set_font ((GnomeFontPreview *) gfsd->preview, font);
}

