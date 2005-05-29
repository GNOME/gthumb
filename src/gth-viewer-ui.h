/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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

#ifndef UI_H
#define UI_H


#include <config.h>
#include "gth-window-actions-callbacks.h"
#include "gth-window-actions-entries.h"
#include "gth-viewer-actions-callbacks.h"
#include "gth-viewer-actions-entries.h"


static const gchar *viewer_window_ui_info = 
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='File' action='FileMenu'>"
"      <menuitem action='File_NewWindow'/>"
"      <separator name='sep01'/>"
"      <menuitem action='Image_OpenWith'/>"
"      <menuitem action='File_OpenFolder'/>"
"      <separator name='sep02'/>"
"      <menuitem action='File_Save'/>"
"      <menuitem action='File_SaveAs'/>"
"      <menuitem action='File_Revert'/>"
"      <separator name='sep03'/>"
"      <menuitem action='File_Print'/>"
"      <separator name='sep04'/>"
"      <menuitem action='File_CloseWindow'/>"
"    </menu>"
"    <menu name='Edit' action='EditMenu'>"
"      <menuitem action='Edit_EditComment'/>"
"      <menuitem action='Edit_EditCategories'/>"
"      <menuitem action='Edit_DeleteComment'/>"
"      <separator/>"
"      <menuitem action='Edit_AddToCatalog'/>"
"    </menu>"
"    <menu name='View' action='ViewMenu'>"
"      <menu name='ShowHide' action='ViewShowHideMenu'>"
"        <menuitem action='View_ShowInfo'/>"
"        <separator/>"
"        <menuitem action='View_Toolbar'/>"
"        <menuitem action='View_Statusbar'/>"
"      </menu>"
"      <separator/>"
"      <menuitem action='View_Fullscreen'/>"
"      <separator/>"
"      <menu name='ZoomType' action='ViewZoomMenu'>"
"        <menuitem action='View_ZoomIn'/>"
"        <menuitem action='View_ZoomOut'/>"
"        <menuitem action='View_Zoom100'/>"
"        <menuitem action='View_ZoomFit'/>"
"        <separator/>"
"        <menuitem action='View_ZoomQualityHigh'/>"
"        <menuitem action='View_ZoomQualityLow'/>"
"      </menu>"
"      <menuitem action='View_PlayAnimation'/>"
"      <menuitem action='View_StepAnimation'/>"
"    </menu>"
"    <menu name='Image' action='ImageMenu'>"
"      <menuitem action='AlterImage_AdjustLevels'/>"
"      <menuitem action='AlterImage_Resize'/>"
"      <menuitem action='AlterImage_Crop'/>"
"      <menu name='Transform' action='ImageTransformMenu'>"
"        <menuitem action='AlterImage_Rotate90'/>"
"        <menuitem action='AlterImage_Rotate90CC'/>"
"        <menuitem action='AlterImage_Flip'/>"
"        <menuitem action='AlterImage_Mirror'/>"
"      </menu>"
"      <separator/>"
"      <menuitem action='AlterImage_Desaturate'/>"
"      <menuitem action='AlterImage_Invert'/>"
"      <menuitem action='AlterImage_ColorBalance'/>"
"      <menuitem action='AlterImage_HueSaturation'/>"
"      <menuitem action='AlterImage_BrightnessContrast'/>"
"      <menuitem action='AlterImage_Posterize'/>"
"      <menu name='Auto' action='ImageAutoMenu'>"
"        <menuitem action='AlterImage_Equalize'/>"
"        <menuitem action='AlterImage_Normalize'/>"
"        <menuitem action='AlterImage_StretchContrast'/>"
"      </menu>"
"    </menu>"
"    <menu name='Tools' action='ToolsMenu'>"
"      <menu name='Wallpaper' action='ToolsWallpaperMenu'>"
"        <menuitem action='Wallpaper_Centered'/>"
"        <menuitem action='Wallpaper_Tiled'/>"
"        <menuitem action='Wallpaper_Scaled'/>"
"        <menuitem action='Wallpaper_Stretched'/>"
"        <separator/>"
"        <menuitem action='Wallpaper_Restore'/>"
"      </menu>"
"    </menu>"
"    <menu name='Help' action='HelpMenu'>"
"      <menuitem action='Help_Help'/>"
"      <menuitem action='Help_Shortcuts'/>"
"      <separator/>"
"      <menuitem action='Help_About'/>"
"    </menu>"
"  </menubar>"
"  <toolbar name='ToolBar'>"
"    <toolitem action='Image_OpenWith'/>"
"    <toolitem action='File_Print'/>"
"    <separator/>"
"    <toolitem action='Edit_EditComment'/>"
"    <toolitem action='Edit_EditCategories'/>"
"    <separator/>"
"    <toolitem action='View_Fullscreen'/>"
"    <separator/>"
"    <toolitem action='View_ZoomIn'/>"
"    <toolitem action='View_ZoomOut'/>"
"    <toolitem action='View_Zoom100'/>"
"    <toolitem action='View_ZoomFit'/>"
"  </toolbar>"
"  <popup name='ImagePopupMenu'>"
"    <menuitem action='View_Fullscreen'/>"
"    <separator/>"
"    <menuitem action='Image_OpenWith'/>"
"    <menuitem action='File_SaveAs'/>"
"    <menuitem action='File_Print'/>"
"    <separator/>"
"    <menu action='ToolsWallpaperMenu'>"
"      <menuitem action='Wallpaper_Centered'/>"
"      <menuitem action='Wallpaper_Tiled'/>"
"      <menuitem action='Wallpaper_Scaled'/>"
"      <menuitem action='Wallpaper_Stretched'/>"
"      <separator/>"
"      <menuitem action='Wallpaper_Restore'/>"
"    </menu>"
"  </popup>"
"</ui>";


#endif /* UI_H */
