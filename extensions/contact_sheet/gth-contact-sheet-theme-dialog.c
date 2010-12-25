/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include "gth-contact-sheet-theme-dialog.h"

#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


static gpointer parent_class = NULL;


struct _GthContactSheetThemeDialogPrivate {
	GtkBuilder *builder;
	GFile      *file;
};


static void
gth_contact_sheet_theme_dialog_finalize (GObject *object)
{
	GthContactSheetThemeDialog *self;

	self = GTH_CONTACT_SHEET_THEME_DIALOG (object);

	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->file);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_contact_sheet_theme_dialog_class_init (GthContactSheetThemeDialogClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthContactSheetThemeDialogPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_contact_sheet_theme_dialog_finalize;
}


static void
gth_contact_sheet_theme_dialog_init (GthContactSheetThemeDialog *self)
{
	GtkWidget *content;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_CONTACT_SHEET_THEME_DIALOG, GthContactSheetThemeDialogPrivate);
	self->priv->builder = _gtk_builder_new_from_file ("contact-sheet-theme-properties.ui", "contact_sheet");
	self->priv->file = NULL;

	gtk_window_set_title (GTK_WINDOW (self), _("Theme Properties"));
	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	gtk_image_set_from_icon_name (GTK_IMAGE (GET_WIDGET ("v_gradient_swap_image")), "tool-mirror", GTK_ICON_SIZE_MENU);
	gtk_image_set_from_icon_name (GTK_IMAGE (GET_WIDGET ("h_gradient_swap_image")), "tool-mirror", GTK_ICON_SIZE_MENU);

	content = _gtk_builder_get_widget (self->priv->builder, "theme_properties");
	gtk_container_set_border_width (GTK_CONTAINER (content), 5);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

	gtk_dialog_add_button (GTK_DIALOG (self),
			       GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self),
			       GTK_STOCK_OK,
			       GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
}


GType
gth_contact_sheet_theme_dialog_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthContactSheetThemeDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_contact_sheet_theme_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthContactSheetThemeDialog),
			0,
			(GInstanceInitFunc) gth_contact_sheet_theme_dialog_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "GthContactSheetThemeDialog",
					       &g_define_type_info,
					       0);
	}

	return type;
}


static void
update_controls_from_theme (GthContactSheetThemeDialog *self,
			    GthContactSheetTheme       *theme)
{
	_g_object_unref (self->priv->file);
	self->priv->file = _g_object_ref (theme->file);

	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), theme->display_name);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("solid_color_radiobutton")), theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("gradient_radiobutton")), theme->background_type != GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("h_gradient_checkbutton")), (theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_HORIZONTAL) || (theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_FULL));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("v_gradient_checkbutton")), (theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_VERTICAL) || (theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_FULL));

	if (theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID) {
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("solid_color_colorpicker")), &theme->background_color1);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("h_gradient_1_colorpicker")), &theme->background_color1);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("h_gradient_2_colorpicker")), &theme->background_color1);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("v_gradient_1_colorpicker")), &theme->background_color1);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("v_gradient_2_colorpicker")), &theme->background_color1);
	}
	else if (theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_FULL) {
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("solid_color_colorpicker")), &theme->background_color1);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("h_gradient_1_colorpicker")), &theme->background_color1);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("h_gradient_2_colorpicker")), &theme->background_color2);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("v_gradient_1_colorpicker")), &theme->background_color3);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("v_gradient_2_colorpicker")), &theme->background_color4);
	}
	else {
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("solid_color_colorpicker")), &theme->background_color1);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("h_gradient_1_colorpicker")), &theme->background_color1);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("h_gradient_2_colorpicker")), &theme->background_color2);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("v_gradient_1_colorpicker")), &theme->background_color1);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("v_gradient_2_colorpicker")), &theme->background_color2);
	}

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("frame_style_combobox")), theme->frame_style);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("frame_colorpicker")), &theme->frame_color);

	gtk_font_button_set_font_name (GTK_FONT_BUTTON (GET_WIDGET ("header_fontpicker")), theme->header_font_name);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("header_colorpicker")), &theme->header_color);

	gtk_font_button_set_font_name (GTK_FONT_BUTTON (GET_WIDGET ("footer_fontpicker")), theme->footer_font_name);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("footer_colorpicker")), &theme->footer_color);

	gtk_font_button_set_font_name (GTK_FONT_BUTTON (GET_WIDGET ("caption_fontpicker")), theme->caption_font_name);
	gtk_color_button_set_color (GTK_COLOR_BUTTON (GET_WIDGET ("caption_colorpicker")), &theme->caption_color);

}


static void
update_theme_preview (GthContactSheetThemeDialog *self)
{

}


static void
gth_contact_sheet_theme_dialog_construct (GthContactSheetThemeDialog *self,
					  GthContactSheetTheme       *theme)
{
	if (theme != NULL)
		update_controls_from_theme (self, theme);
	update_theme_preview (self);
}


GtkWidget *
gth_contact_sheet_theme_dialog_new (GthContactSheetTheme *theme)
{
	GthContactSheetThemeDialog *self;

	self = g_object_new (GTH_TYPE_CONTACT_SHEET_THEME_DIALOG, NULL);
	gth_contact_sheet_theme_dialog_construct (self, theme);

	return (GtkWidget *) self;
}


GthContactSheetTheme *
gth_contact_sheet_theme_dialog_get_theme (GthContactSheetThemeDialog *self)
{
	GthContactSheetTheme *theme;

	theme = gth_contact_sheet_theme_new ();
	theme->file = _g_object_ref (self->priv->file);
	theme->display_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("name_entry"))));

	/* background */

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("solid_color_radiobutton")))) {
		theme->background_type = GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID;
		gtk_color_button_get_color (GTK_COLOR_BUTTON (GET_WIDGET ("solid_color_colorpicker")), &theme->background_color1);
	}
	else {
		gboolean h_gradient_active = FALSE;
		gboolean v_gradient_active = FALSE;

		h_gradient_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("h_gradient_checkbutton")));
		v_gradient_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("v_gradient_checkbutton")));

		if (h_gradient_active && v_gradient_active) {
			theme->background_type = GTH_CONTACT_SHEET_BACKGROUND_TYPE_FULL;
			gtk_color_button_get_color (GTK_COLOR_BUTTON (GET_WIDGET ("h_gradient_1_colorpicker")), &theme->background_color1);
			gtk_color_button_get_color (GTK_COLOR_BUTTON (GET_WIDGET ("h_gradient_2_colorpicker")), &theme->background_color2);
			gtk_color_button_get_color (GTK_COLOR_BUTTON (GET_WIDGET ("v_gradient_1_colorpicker")), &theme->background_color3);
			gtk_color_button_get_color (GTK_COLOR_BUTTON (GET_WIDGET ("v_gradient_2_colorpicker")), &theme->background_color4);
		}
		else if (h_gradient_active) {
			theme->background_type = GTH_CONTACT_SHEET_BACKGROUND_TYPE_HORIZONTAL;
			gtk_color_button_get_color (GTK_COLOR_BUTTON (GET_WIDGET ("h_gradient_1_colorpicker")), &theme->background_color1);
			gtk_color_button_get_color (GTK_COLOR_BUTTON (GET_WIDGET ("h_gradient_2_colorpicker")), &theme->background_color2);
		}
		else if (v_gradient_active) {
			theme->background_type = GTH_CONTACT_SHEET_BACKGROUND_TYPE_VERTICAL;
			gtk_color_button_get_color (GTK_COLOR_BUTTON (GET_WIDGET ("v_gradient_1_colorpicker")), &theme->background_color1);
			gtk_color_button_get_color (GTK_COLOR_BUTTON (GET_WIDGET ("v_gradient_2_colorpicker")), &theme->background_color2);
		}
	}

	/* frame */

	theme->frame_style = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("frame_style_combobox")));
	gtk_color_button_get_color (GTK_COLOR_BUTTON (GET_WIDGET ("frame_colorpicker")), &theme->frame_color);

	/* header */

	theme->header_font_name = g_strdup (gtk_font_button_get_font_name (GTK_FONT_BUTTON (GET_WIDGET ("header_fontpicker"))));
	gtk_color_button_get_color (GTK_COLOR_BUTTON (GET_WIDGET ("header_colorpicker")), &theme->header_color);

	/* footer */

	theme->footer_font_name = g_strdup (gtk_font_button_get_font_name (GTK_FONT_BUTTON (GET_WIDGET ("footer_fontpicker"))));
	gtk_color_button_get_color (GTK_COLOR_BUTTON (GET_WIDGET ("footer_colorpicker")), &theme->footer_color);

	/* caption */

	theme->caption_font_name = g_strdup (gtk_font_button_get_font_name (GTK_FONT_BUTTON (GET_WIDGET ("caption_fontpicker"))));
	gtk_color_button_get_color (GTK_COLOR_BUTTON (GET_WIDGET ("caption_colorpicker")), &theme->caption_color);

	return theme;
}
