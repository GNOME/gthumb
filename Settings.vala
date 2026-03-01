// Schemas

const string GTHUMB_SCHEMA = "app.gthumb.gthumb";
const string GTHUMB_VIEWER_SCHEMA = GTHUMB_SCHEMA + ".viewers.common";
const string GTHUMB_IMAGES_SCHEMA = GTHUMB_SCHEMA + ".viewers.images";
const string GTHUMB_VIDEOS_SCHEMA = GTHUMB_SCHEMA + ".viewers.videos";
const string GTHUMB_PNG_SAVER_SCHEMA = GTHUMB_SCHEMA + ".savers.png";
const string GTHUMB_JPEG_SAVER_SCHEMA = GTHUMB_SCHEMA + ".savers.jpeg";
const string GTHUMB_WEBP_SAVER_SCHEMA = GTHUMB_SCHEMA + ".savers.webp";
const string GTHUMB_AVIF_SAVER_SCHEMA = GTHUMB_SCHEMA + ".savers.avif";
const string GTHUMB_TIFF_SAVER_SCHEMA = GTHUMB_SCHEMA + ".savers.tiff";
const string GTHUMB_JXL_SAVER_SCHEMA = GTHUMB_SCHEMA + ".savers.jxl";
const string GTHUMB_TERMINAL_SCHEMA = GTHUMB_SCHEMA + ".terminal";
const string GTHUMB_RENAME_SCHEMA = GTHUMB_SCHEMA + ".rename";
const string GTHUMB_ROTATE_SCHEMA = GTHUMB_SCHEMA + ".rotate";
const string GTHUMB_RESIZE_SCHEMA = GTHUMB_SCHEMA + ".resize";
const string GTHUMB_CONVERT_SCHEMA = GTHUMB_SCHEMA + ".convert";
const string GTHUMB_PRINT_SCHEMA = GTHUMB_SCHEMA + ".print";
const string GTHUMB_CROP_SCHEMA = GTHUMB_SCHEMA + ".crop";
const string GTHUMB_IMPORT_SCHEMA = GTHUMB_SCHEMA + ".import";

const string PREF_GENERAL_STORE_METADATA_IN_FILES = "store-metadata-in-files";
const string PREF_GENERAL_SHOW_FORMAT_OPTIONS = "show-format-options";

const string PREF_BROWSER_HOME_FOLDER = "home-folder";
const string PREF_BROWSER_RESTORE_SESSION = "restore-session";
const string PREF_BROWSER_SESSION_LOCATION = "session-location";
const string PREF_BROWSER_SESSION_CURRENT_FILE = "session-current-file";
const string PREF_BROWSER_REUSE_ACTIVE_WINDOW = "reuse-active-window";
const string PREF_BROWSER_GENERAL_FILTER = "general-filter";
const string PREF_BROWSER_SHOW_HIDDEN_FILES = "show-hidden-files";
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
const string PREF_BROWSER_LAST_CATALOG = "last-catalog";
const string PREF_BROWSER_ADD_TO_CATALOG_SHOW_DESTINATION = "add-to-catalog-show-destination";
const string PREF_BROWSER_CLIPBOARD_TYPE = "clipboard-type";
const string PREF_BROWSER_LAST_EXTENSION = "last-extension";

const string PREF_VIEWER_SIDEBAR_VISIBLE = "sidebar-visible";
const string PREF_VIEWER_FILE_LIST_VISIBLE = "file-list-visible";
const string PREF_VIEWER_FILE_LIST_SIZE = "file-list-size";

const string PREF_IMAGE_ZOOM_TYPE = "zoom-type";
const string PREF_IMAGE_SCROLL_ACTION = "scroll-action";
const string PREF_IMAGE_TRANSPARENCY = "transparency";
const string PREF_IMAGE_APPLY_ICC_PROFILE = "apply-icc-profile";

const string PREF_VIDEO_SCREENSHOT_LOCATION = "screenshot-location";
const string PREF_VIDEO_SCREENSHOT_TYPE = "screenshot-type";
const string PREF_VIDEO_SCROLL_ACTION = "scroll-action";
const string PREF_VIDEO_VOLUME = "volume";
const string PREF_VIDEO_MUTE = "mute";
const string PREF_VIDEO_ZOOM_TO_FIT = "zoom-to-fit";

const string PREF_PNG_COMPRESSION_LEVEL = "compression-level";

const string PREF_JPEG_DEFAULT_EXT = "default-ext";
const string PREF_JPEG_QUALITY = "quality";
const string PREF_JPEG_SMOOTHING = "smoothing";
const string PREF_JPEG_OPTIMIZE = "optimize";
const string PREF_JPEG_PROGRESSIVE = "progressive";

const string PREF_WEBP_QUALITY = "quality";
const string PREF_WEBP_METHOD = "method";
const string PREF_WEBP_LOSSLESS = "lossless";

const string PREF_AVIF_QUALITY = "quality";
const string PREF_AVIF_LOSSLESS = "lossless";

const string PREF_TIFF_DEFAULT_EXT = "default-ext";
const string PREF_TIFF_COMPRESSION = "compression";
const string PREF_TIFF_HRESOLUTION = "horizontal-resolution";
const string PREF_TIFF_VRESOLUTION = "vertical-resolution";

const string PREF_JXL_EFFORT = "effort";
const string PREF_JXL_DISTANCE = "distance";
const string PREF_JXL_LOSSLESS = "lossless";

const string PREF_TERMINAL_COMMAND = "command";

const string PREF_RENAME_TEMPLATE = "template";
const string PREF_RENAME_FIRST_INDEX = "first-index";
const string PREF_RENAME_SORT_NAME = "sort-name";
const string PREF_RENAME_REVERSE_ORDER = "reverse-order";
const string PREF_RENAME_TEXT_TRANSFORM = "text-transform";

const string PREF_ROTATE_BACKGROUND = "background";
const string PREF_ROTATE_SIZE = "size";

const string PREF_RESIZE_WIDTH = "width";
const string PREF_RESIZE_HEIGHT = "height";
const string PREF_RESIZE_UNIT = "unit";
const string PREF_RESIZE_KEEP_ASPECT_RATIO = "keep-aspect-ratio";
const string PREF_RESIZE_FORMAT = "format";

const string PREF_CONVERT_FORMAT = "format";

const string PREF_PRINT_LAYOUT_ROWS = "rows";
const string PREF_PRINT_LAYOUT_COLUMNS = "columns";
const string PREF_PRINT_UNIT = "unit";
const string PREF_PRINT_OUTPUT_DIR = "output-dir";

const string PREF_CROP_CUSTOM_RATIO = "custom-ratio";

const string PREF_IMPORT_DESTINATION = "destination";
const string PREF_IMPORT_AUTOMATIC_SUBFOLDER = "automatic-subfolder";
const string PREF_IMPORT_AUTOMATIC_SUBFOLDER_DATE = "automatic-subfolder-date";
const string PREF_IMPORT_AUTOMATIC_SUBFOLDER_FORMAT = "automatic-subfolder-format";
const string PREF_IMPORT_DELETE_IMPORTED = "delete-imported";

class Gth.Settings {
	public static File get_home_folder (GLib.Settings settings) {
		File location = Gth.Settings.get_file (settings, PREF_BROWSER_HOME_FOLDER);
		if (location == null) {
			location = File.new_for_path (Environment.get_home_dir ());
		}
		return location;
	}

	public static File? get_startup_location (GLib.Settings settings) {
		File location = null;
		if (settings.get_boolean (PREF_BROWSER_RESTORE_SESSION)) {
			location = Gth.Settings.get_file (settings, PREF_BROWSER_SESSION_LOCATION);
			if (location == null) {
				var path = Environment.get_user_special_dir (UserDirectory.PICTURES);
				if (path != null) {
					location = File.new_for_path (path);
				}
			}
		}
		if (location == null) {
			location = get_home_folder (settings);
		}
		return location;
	}

	public static File? get_file (GLib.Settings settings, string key) {
		var uri = settings.get_string (key);
		if (Strings.empty (uri)) {
			return null;
		}
		var home_uri = Util.uri_from_path (Environment.get_home_dir ());
		uri = uri.replace ("file://~", home_uri);
		return File.new_for_uri (uri);
	}
}
