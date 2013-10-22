/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2004-2009 Free Software Foundation, Inc.
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

#ifndef GTH_BROWSER_UI_H
#define GTH_BROWSER_UI_H

#include <config.h>

static const char *fixed_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Edit' action='EditMenu'>"
"      <placeholder name='File_Actions_1'/>"
"      <separator/>"
"      <placeholder name='File_Actions'/>"
"      <separator/>"
"      <placeholder name='List_Actions'/>"
"      <separator/>"
"      <placeholder name='Folder_Actions'/>"
"      <placeholder name='Folder_Actions_2'/>"
"      <separator/>"
"      <placeholder name='Edit_Actions'/>"
"    </menu>"
"    <menu name='View' action='ViewMenu'>"
"      <menuitem action='View_Stop'/>"
"      <menuitem action='View_Reload'/>"
"      <separator/>"
"      <menuitem action='View_Statusbar'/>"
"      <placeholder name='View_Bars'/>"
"      <separator/>"
"      <placeholder name='View_Actions'/>"
"      <separator/>"
"      <placeholder name='File_Actions'/>"
"      <separator/>"
"      <menuitem action='View_ShowHiddenFiles'/>"
"      <menuitem action='View_Sort_By'/>"
"      <menuitem action='View_Filters'/>"
"      <separator/>"
"      <placeholder name='Folder_Actions'/>"
"    </menu>"
"    <placeholder name='OtherMenus'/>"
"  </menubar>"

"  <toolbar name='Fullscreen_ToolBar'>"
"    <placeholder name='ViewerCommands'/>"
"    <separator/>"
"    <placeholder name='Edit_Actions'/>"
"    <placeholder name='Edit_Actions_2'/>"
"    <separator expand='true'/>"
"    <placeholder name='ViewerCommandsSecondary'/>"
"  </toolbar>"

"  <popup name='FileListPopup'>"
"    <placeholder name='Screen_Actions'/>"
"    <separator/>"
"    <menu name='OpenWith' action='OpenWithMenu'>"
"    </menu>"
"    <placeholder name='Open_Actions'/>"
"    <separator/>"
"    <placeholder name='File_Actions'/>"
"    <separator/>"
"    <placeholder name='Folder_Actions'/>"
"    <separator/>"
"    <placeholder name='Folder_Actions2'/>"
"    <separator/>"
"    <placeholder name='File_LastActions'/>"
"  </popup>"

"  <popup name='FilePopup'>"
"    <placeholder name='Screen_Actions'/>"
"    <separator/>"
"    <menu name='OpenWith' action='OpenWithMenu'>"
"    </menu>"
"    <placeholder name='Open_Actions'/>"
"    <separator/>"
"    <placeholder name='File_Actions'/>"
"    <separator/>"
"    <placeholder name='Folder_Actions'/>"
"    <separator/>"
"    <placeholder name='Folder_Actions2'/>"
"    <separator/>"
"    <placeholder name='File_LastActions'/>"
"  </popup>"

"  <popup name='FolderListPopup'>"
"    <menuitem action='Folder_Open'/>"
"    <menuitem action='Folder_OpenInNewWindow'/>"
"    <placeholder name='OpenCommands'/>"
"    <separator/>"
"    <placeholder name='SourceCommands'/>"
"  </popup>"

"</ui>";


static const char *browser_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Edit' action='EditMenu'>"
"      <placeholder name='List_Actions'>"
"        <menuitem action='Edit_SelectAll'/>"
"      </placeholder>"
"      <placeholder name='Edit_Actions'>"
"      </placeholder>"
"    </menu>"
"    <menu name='View' action='ViewMenu'>"
"      <placeholder name='View_Bars'>"
"        <menuitem action='View_Sidebar'/>"
"        <menuitem action='View_Filterbar'/>"
"      </placeholder>"
"      <placeholder name='Folder_Actions'>"
"        <menuitem action='View_Thumbnails'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"</ui>";


static const char *viewer_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='View' action='ViewMenu'>"
"      <placeholder name='View_Bars'>"
"        <menuitem action='View_Thumbnail_List'/>"
"      </placeholder>"
"      <placeholder name='View_Actions'>"
"        <menuitem action='View_ShrinkWrap'/>"
"      </placeholder>"
"      <placeholder name='Folder_Actions'>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"</ui>";

#endif /* GTH_BROWSER_UI_H */
