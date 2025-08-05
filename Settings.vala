// Schemas

const string GTHUMB_SCHEMA = "app.gthumb.gthumb";
const string GTHUMB_VIEWER_SCHEMA = GTHUMB_SCHEMA + ".viewers.common";
const string GTHUMB_IMAGES_SCHEMA = GTHUMB_SCHEMA + ".viewers.images";
const string GTHUMB_VIDEOS_SCHEMA = GTHUMB_SCHEMA + ".viewers.videos";
const string GTHUMB_PNG_SAVER_SCHEMA = GTHUMB_SCHEMA + ".savers.png";
const string GTHUMB_JPEG_SAVER_SCHEMA = GTHUMB_SCHEMA + ".savers.jpeg";

const string PREF_GENERAL_STORE_METADATA_IN_FILES = "store-metadata-in-files";

const string PREF_BROWSER_GO_TO_LAST_LOCATION = "go-to-last-location";
const string PREF_BROWSER_USE_STARTUP_LOCATION = "use-startup-location";
const string PREF_BROWSER_STARTUP_LOCATION = "startup-location";
const string PREF_BROWSER_STARTUP_CURRENT_FILE = "startup-current-file";
const string PREF_BROWSER_REUSE_ACTIVE_WINDOW = "reuse-active-window";
const string PREF_BROWSER_GENERAL_FILTER = "general-filter";
const string PREF_BROWSER_SHOW_HIDDEN_FILES = "show-hidden-files";
const string PREF_BROWSER_FAST_FILE_TYPE = "fast-file-type";
const string PREF_BROWSER_THUMBNAIL_SIZE = "thumbnail-size";
const string PREF_BROWSER_THUMBNAIL_CAPTION = "thumbnail-caption";
const string PREF_BROWSER_SORT_TYPE = "sort-type";
const string PREF_BROWSER_SORT_INVERSE = "sort-inverse";
const string PREF_BROWSER_WINDOW_WIDTH = "window-width";
const string PREF_BROWSER_WINDOW_HEIGHT = "window-height";
const string PREF_BROWSER_WINDOW_MAXIMIZED = "maximized";
const string PREF_BROWSER_SIDEBAR_VISIBLE = "sidebar-visible";
const string PREF_BROWSER_SIDEBAR_WIDTH = "sidebar-width";
const string PREF_BROWSER_PROPERTIES_VISIBLE = "properties-visible";
const string PREF_BROWSER_PROPERTIES_WIDTH = "properties-width";
const string PREF_BROWSER_FOLDER_TREE_SORT_TYPE = "folder-tree-sort-type";
const string PREF_BROWSER_FOLDER_TREE_SORT_INVERSE = "folder-tree-sort-inverse";
const string PREF_BROWSER_OPEN_IN_FULLSCREEN = "open-files-in-fullscreen";

const string PREF_VIEWER_SIDEBAR_VISIBLE = "sidebar-visible";

const string PREF_IMAGE_ZOOM_TYPE = "zoom-type";

const string PREF_VIDEO_SCREESHOT_LOCATION = "screenshot-location";
const string PREF_VIDEO_VOLUME = "volume";
const string PREF_VIDEO_MUTE = "mute";
const string PREF_VIDEO_ZOOM_TO_FIT = "zoom-to-fit";

const string PREF_PNG_COMPRESSION_LEVEL = "compression-level";

const string PREF_JPEG_DEFAULT_EXT = "default-ext";
const string PREF_JPEG_QUALITY = "quality";
const string PREF_JPEG_SMOOTHING = "smoothing";
const string PREF_JPEG_OPTIMIZE = "optimize";
const string PREF_JPEG_PROGRESSIVE = "progressive";

class Gth.Settings {
	public static File? get_startup_location (GLib.Settings settings) {
		File location = null;
		if (settings.get_boolean (PREF_BROWSER_USE_STARTUP_LOCATION)
			|| settings.get_boolean (PREF_BROWSER_GO_TO_LAST_LOCATION))
		{
			location = Gth.Settings.get_file (settings, PREF_BROWSER_STARTUP_LOCATION);
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
		if (Strings.empty (uri))
			return null;
		var home_uri = Util.uri_from_path (Environment.get_home_dir ());
		uri = uri.replace ("file://~", home_uri);
		return File.new_for_uri (uri);
	}
}
