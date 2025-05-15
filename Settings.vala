// Schemas

const string GTHUMB_SCHEMA = "org.gnome.gthumb";
const string GTHUMB_GENERAL_SCHEMA = GTHUMB_SCHEMA + ".general";
const string GTHUMB_DATA_MIGRATION_SCHEMA = GTHUMB_SCHEMA + ".data-migration";
const string GTHUMB_BROWSER_SCHEMA = GTHUMB_SCHEMA + ".browser";
const string GTHUMB_DIALOGS_SCHEMA = GTHUMB_SCHEMA + ".dialogs";
const string GTHUMB_MESSAGES_SCHEMA = GTHUMB_DIALOGS_SCHEMA + ".messages";
const string GTHUMB_ADD_TO_CATALOG_SCHEMA = GTHUMB_DIALOGS_SCHEMA + ".add-to-catalog";
const string GTHUMB_SAVE_FILE_SCHEMA = GTHUMB_DIALOGS_SCHEMA + ".save-file";

// General

const string PREF_GENERAL_ACTIVE_EXTENSIONS = "active-extensions";
const string PREF_GENERAL_STORE_METADATA_IN_FILES = "store-metadata-in-files";

// Dada migration

const string PREF_DATA_MIGRATION_CATALOGS_2_10 = "catalogs-2-10";

// Browser

const string PREF_BROWSER_GO_TO_LAST_LOCATION = "go-to-last-location";
const string PREF_BROWSER_USE_LOCATIOn = "use-startup-location";
const string PREF_BROWSER_LOCATIOn = "startup-location";
const string PREF_BROWSER_STARTUP_CURRENT_FILE = "startup-current-file";
const string PREF_BROWSER_GENERAL_FILTER = "general-filter";
const string PREF_BROWSER_SHOW_HIDDEN_FILES = "show-hidden-files";
const string PREF_BROWSER_FAST_FILE_TYPE = "fast-file-type";
const string PREF_BROWSER_SAVE_THUMBNAILS = "save-thumbnails";
const string PREF_BROWSER_THUMBNAIL_SIZE = "thumbnail-size";
const string PREF_BROWSER_THUMBNAIL_LIMIT = "thumbnail-limit";
const string PREF_BROWSER_THUMBNAIL_CAPTION = "thumbnail-caption";
const string PREF_BROWSER_SINGLE_CLICK_ACTIVATION = "single-click-activation";
const string PREF_BROWSER_OPEN_FILES_IN_FULLSCREEN = "open-files-in-fullscreen";
const string PREF_BROWSER_SORT_TYPE = "sort-type";
const string PREF_BROWSER_SORT_INVERSE = "sort-inverse";
const string PREF_BROWSER_WINDOW_WIDTH = "window-width";
const string PREF_BROWSER_WINDOW_HEIGHT = "window-height";
const string PREF_BROWSER_WINDOW_MAXIMIZED = "maximized";
const string PREF_BROWSER_STATUSBAR_VISIBLE = "statusbar-visible";
const string PREF_BROWSER_SIDEBAR_VISIBLE = "sidebar-visible";
const string PREF_BROWSER_SIDEBAR_SECTIONS = "sidebar-sections";
const string PREF_BROWSER_PROPERTIES_VISIBLE = "properties-visible";
const string PREF_BROWSER_PROPERTIES_ON_THE_RIGHT = "properties-on-the-right";
const string PREF_BROWSER_THUMBNAIL_LIST_VISIBLE = "thumbnail-list-visible";
const string PREF_BROWSER_BROWSER_SIDEBAR_WIDTH = "browser-sidebar-width";
const string PREF_BROWSER_VIEWER_SIDEBAR = "viewer-sidebar";
const string PREF_BROWSER_VIEWER_THUMBNAILS_ORIENT = "viewer-thumbnails-orientation";
const string PREF_BROWSER_REUSE_ACTIVE_WINDOW = "reuse-active-window";
const string PREF_FULLSCREEN_THUMBNAILS_VISIBLE = "fullscreen-thumbnails-visible";
const string PREF_FULLSCREEN_SIDEBAR = "fullscreen-sidebar";
const string PREF_VIEWER_SCROLL_ACTION = "scroll-action";
const string PREF_BROWSER_FAVORITE_PROPERTIES = "favorite-properties";
const string PREF_BROWSER_FOLDER_TREE_SORT_TYPE = "folder-tree-sort-type";
const string PREF_BROWSER_FOLDER_TREE_SORT_INVERSE = "folder-tree-sort-inverse";

// Add to catalog

const string PREF_ADD_TO_CATALOG_LAST_CATALOG = "last-catalog";
const string PREF_ADD_TO_CATALOG_VIEW = "view";

// Save file

const string PREF_SAVE_FILE_SHOW_OPTIONS = "show-options";

// Messages

const string PREF_MSG_CANNOT_MOVE_TO_TRASH = "cannot-move-to-trash";
const string PREF_MSG_SAVE_MODIFIED_IMAGE = "save-modified-image";
const string PREF_MSG_CONFIRM_DELETION = "confirm-deletion";

// Foreign schemas

const string GNOME_DESKTOP_BACKGROUND_SCHEMA = "org.gnome.desktop.background";
const string PREF_BACKGROUND_PICTURE_URI = "picture-uri";
const string PREF_BACKGROUND_PICTURE_OPTIONS = "picture-options";

const string GNOME_DESKTOP_INTERFACE_SCHEMA = "org.gnome.desktop.interface";

class Gth.Settings {
	public static File? get_startup_location (GLib.Settings settings) {
		File location = null;
		if (settings.get_boolean (PREF_BROWSER_USE_LOCATIOn)
			|| settings.get_boolean (PREF_BROWSER_GO_TO_LAST_LOCATION))
		{
			location = Gth.Settings.get_file (settings, PREF_BROWSER_LOCATIOn);
			if (location == null) {
				var path = Environment.get_user_special_dir (UserDirectory.PICTURES);
				if (path != null) {
					location = File.new_for_path (path);
				}
			}
		}
		if (location == null) {
			location = File.new_for_path (Environment.get_home_dir ());
		}
		return location;
	}

	public static File? get_file (GLib.Settings settings, string key) {
		var uri = settings.get_string (key);
		if (uri == null)
			return null;
		var home_uri = Gth.Uri.uri_from_path (Environment.get_home_dir ());
		uri = uri.replace ("file://~", home_uri);
		return File.new_for_uri (uri);
	}
}
