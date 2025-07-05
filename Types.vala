const string APP_DIR = "gthumb";
const string HISTORY_FILE = "history.xbel";
const string BOOKMARKS_FILE = "bookmarks.xbel";
const string FILTERS_FILE = "filters.xml";

const int HORIZONTAL_SPACING = 12; // pixels
const int VERTICAL_SPACING = 12; // pixels
const int CONTAINER_H_PADDING = 12; // pixels
const int CONTAINER_V_PADDING = 12; // pixels

const string ACCESS_ATTRIBUTES = "access::*";
const string STANDARD_ATTRIBUTES =
	FileAttribute.STANDARD_TYPE + "," +
	FileAttribute.STANDARD_IS_HIDDEN + "," +
	FileAttribute.STANDARD_IS_BACKUP + "," +
	FileAttribute.STANDARD_NAME + "," +
	FileAttribute.STANDARD_DISPLAY_NAME + "," +
	FileAttribute.STANDARD_EDIT_NAME + "," +
	FileAttribute.STANDARD_SIZE + "," +
	FileAttribute.TIME_CREATED + "," +
	FileAttribute.TIME_CREATED_USEC + "," +
	FileAttribute.TIME_MODIFIED + "," +
	FileAttribute.TIME_MODIFIED_USEC + "," +
	ACCESS_ATTRIBUTES;
const string STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE =
	STANDARD_ATTRIBUTES + "," +
	FileAttribute.STANDARD_FAST_CONTENT_TYPE;
const string STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE =
	STANDARD_ATTRIBUTES + "," +
	FileAttribute.STANDARD_FAST_CONTENT_TYPE + "," +
	FileAttribute.STANDARD_CONTENT_TYPE;
const string EMBLEMS_ATTRIBUTE = "gth::file::emblems";
const string VOLUME_ATTRIBUTE = "gth::volume";

public struct Gth.SortInfo {
	string id;
	string display_name;
	string required_attributes;
	CompareFunc<Gth.FileData> cmp_func;
}

public struct Gth.TestInfo {
	string id;
	string display_name;
	GLib.Type test_type;
}

public const Gth.MetadataFlags METADATA_ALLOW_EVERYWHERE = Gth.MetadataFlags.ALLOW_IN_FILE_LIST | Gth.MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | Gth.MetadataFlags.ALLOW_IN_PRINT;

public delegate void Gth.IdleFunc ();

public enum Gth.ThumbnailState {
	ICON,
	LOADING,
	BROKEN,
	LOADED;
}

public enum Gth.LoadAction {
	OPEN,
	OPEN_AS_ROOT,
	OPEN_FROM_HISTORY,
	OPEN_SUBFOLDER;

	public bool changes_current_folder () {
		return this <= OPEN_SUBFOLDER;
	}
}

public enum Gth.BrowserSidebarState {
	NONE,
	FILES,
	CATALOGS,
}

public enum Gth.SidebarState {
	NONE,
	PROPERTIES,
	TOOLS,
}

public struct Gth.Sort {
	string name;
	bool inverse;
}
