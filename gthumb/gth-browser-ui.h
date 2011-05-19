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
"    <menu name='File' action='FileMenu'>"
"      <menuitem action='File_NewWindow'/>"
"      <separator/>"
"      <menuitem action='File_Save'/>"
"      <menuitem action='File_SaveAs'/>"
"      <menuitem action='File_Revert'/>"
"      <placeholder name='File_Actions'/>"
"      <placeholder name='File_Actions_2'/>"
"      <separator/>"
"      <placeholder name='Folder_Actions'/>"
"      <separator/>"
"      <menu name='Import' action='ImportMenu'>"
"        <placeholder name='Misc_Actions'/>"
"        <separator/>"
"        <placeholder name='Web_Services'/>"
"      </menu>"
"      <menu name='Export' action='ExportMenu'>"
"        <placeholder name='Web_Services'/>"
"        <separator/>"
"        <placeholder name='Misc_Actions'/>"
"      </menu>"
"      <placeholder name='Misc_Actions'/>"
"      <separator/>"
"      <menuitem action='File_CloseWindow'/>"
"    </menu>"
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
"      <separator/>"
"      <menuitem action='Edit_Preferences'/>"
"    </menu>"
"    <menu name='View' action='ViewMenu'>"
"      <menuitem action='View_Stop'/>"
"      <menuitem action='View_Reload'/>"
"      <separator/>"
"      <menuitem action='View_Toolbar'/>"
"      <menuitem action='View_Statusbar'/>"
"      <placeholder name='View_Bars'/>"
"      <separator/>"
"      <menuitem action='View_Fullscreen'/>"
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
"    <menu name='Go' action='GoMenu'>"
"      <menuitem action='Go_Back'/>"
"      <menuitem action='Go_Forward'/>"
"      <menuitem action='Go_Up'/>"
"      <separator name='BeforeEntryPointList'/>"
"      <placeholder name='EntryPointList'/>"
"      <separator name='EntryPointListSeparator'/>"
"      <menuitem action='Go_Clear_History'/>"
"      <separator name='BeforeHistoryList'/>"
"      <placeholder name='HistoryList'/>"
"    </menu>"
"    <placeholder name='OtherMenus'/>"
"    <menu name='Help' action='HelpMenu'>"
"      <menuitem action='Help_Help'/>"
"      <menuitem action='Help_Shortcuts'/>"
"      <separator/>"
"      <menuitem name='About' action='Help_About'/>"
"    </menu>"
"  </menubar>"

"  <toolbar name='ToolBar'>"
"    <toolitem action='View_Stop'/>"
"    <separator/>"
"    <placeholder name='Export_Actions'/>"
"    <placeholder name='SourceCommands'/>"
"    <separator/>"
"    <placeholder name='BrowserCommands'/>"
"    <separator/>"
"    <placeholder name='Edit_Actions'/>"
"  </toolbar>"

"  <toolbar name='ViewerToolBar'>"
"    <toolitem action='View_BrowserMode'/>"
"    <separator/>"
"    <toolitem action='View_Prev'/>"
"    <toolitem action='View_Next'/>"
"    <separator/>"
"    <placeholder name='ViewerCommands'/>"
"    <separator/>"
"    <placeholder name='Edit_Actions'/>"
"    <separator expand='true'/>"
"    <placeholder name='ViewerCommandsSecondary'/>"
"    <toolitem action='Viewer_Properties'/>"
"  </toolbar>"

"  <toolbar name='Fullscreen_ToolBar'>"
"    <toolitem action='View_Leave_Fullscreen'/>"
"    <separator/>"
"    <toolitem action='View_Prev'/>"
"    <toolitem action='View_Next'/>"
"    <separator/>"
"    <placeholder name='ViewerCommands'/>"
"    <separator/>"
"    <placeholder name='Edit_Actions'/>"
"    <separator expand='true'/>"
"    <placeholder name='ViewerCommandsSecondary'/>"
"    <toolitem action='Viewer_Properties'/>"
"  </toolbar>"

"  <popup name='GoBackHistoryPopup'>"
"  </popup>"

"  <popup name='GoForwardHistoryPopup'>"
"  </popup>"

"  <popup name='GoParentPopup'>"
"  </popup>"

"  <popup name='FileListPopup'>"
"    <menuitem name='Open' action='File_Open'/>"
"    <menu name='OpenWith' action='OpenWithMenu'>"
"    </menu>"
"    <placeholder name='Open_Actions'/>"
"    <separator/>"
"    <placeholder name='Screen_Actions'>"
"      <menuitem action='View_Fullscreen'/>"
"    </placeholder>"
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
"    <menu name='OpenWith' action='OpenWithMenu'>"
"    </menu>"
"    <separator/>"
"    <placeholder name='Open_Actions'/>"
"    <placeholder name='Screen_Actions'>"
"      <menuitem action='View_Fullscreen'/>"
"    </placeholder>"
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

"  <accelerator action=\"Go_Home\" />"

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
"  <toolbar name='ToolBar'>"
"    <placeholder name='BrowserCommands'>"
"      <toolitem action='View_Fullscreen'/>"
"    </placeholder>"
"  </toolbar>"
"</ui>";


static const char *viewer_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='View' action='ViewMenu'>"
"      <placeholder name='View_Bars'>"
"        <menuitem action='View_Thumbnail_List'/>"
"      </placeholder>"
"      <placeholder name='Folder_Actions'>"
"        <menuitem action='View_BrowserMode'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"</ui>";

#endif /* GTH_BROWSER_UI_H */
