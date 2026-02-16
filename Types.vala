const string APP_DIR = "gthumb";
const string HISTORY_FILE = "history.xbel";
const string BOOKMARKS_FILE = "bookmarks.xbel";
const string FILTERS_FILE = "filters.xml";
const string SCRIPTS_FILE = "scripts.xml";
const string TOOLS_FILE = "tools.xml";
const string SHORTCUTS_FILE = "shortcuts.xml";
const string SELECTIONS_FILE = "selections.xml";

const int HORIZONTAL_SPACING = 12; // pixels
const int VERTICAL_SPACING = 12; // pixels
const int CONTAINER_H_PADDING = 12; // pixels
const int CONTAINER_V_PADDING = 12; // pixels
const int MAX_SIDEBAR_WIDTH = 700;
const int MAX_SIZE_FOR_PRELOADER = 5000 * 5000;

const string REQUIRED_ATTRIBUTES =
	FileAttribute.STANDARD_TYPE + "," +
	FileAttribute.STANDARD_IS_HIDDEN + "," +
	FileAttribute.STANDARD_IS_BACKUP + "," +
	FileAttribute.STANDARD_NAME + "," +
	FileAttribute.STANDARD_DISPLAY_NAME + "," +
	FileAttribute.STANDARD_EDIT_NAME + "," +
	FileAttribute.ID_FILE;
const string ACCESS_ATTRIBUTES = "access::*";
const string STANDARD_ATTRIBUTES =
	REQUIRED_ATTRIBUTES + "," +
	FileAttribute.STANDARD_SIZE + "," +
	FileAttribute.STANDARD_ICON + "," +
	FileAttribute.STANDARD_SYMBOLIC_ICON + "," +
	FileAttribute.TIME_CREATED + "," +
	FileAttribute.TIME_CREATED_USEC + "," +
	FileAttribute.TIME_CHANGED + "," +
	FileAttribute.TIME_CHANGED_USEC + "," +
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
const string EMBLEMS_ATTRIBUTE = "Private::File::Emblems";
const string VOLUME_ATTRIBUTE = "Private::Volume";
const string DEFAULT_STRFTIME_FORMAT = "%Y-%m-%d--%H.%M.%S";
const Graphene.Size[] OTHER_RATIOS = {
	{ 16, 9 },
	{ 16, 10 },
	{ 21, 9 },
	{ 5, 4 },
	{ 4, 3 },
	{ 3, 2 },
};

// Video types not matching 'video/*'
const string[] Other_Video_Types = {
	"application/ogg",
	"application/x-matroska",
	"application/vnd.ms-asf",
	"application/vnd.rn-realmedia",
};

namespace Dev.Enum {
	public static int index_of (string[] arr, string val) {
		for (var i = 0; i < arr.length; i++) {
			if (arr[i] == val)
				return i;
		}
		return -1;
	}

	public static bool set_state (string[] states, string? state, ref int value) {
		if (state == null)
			return false;
		var idx = Dev.Enum.index_of (states, state);
		if (idx < 0)
			return false;
		value = idx;
		return true;
	}
}

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
	OPEN_SUBFOLDER,
	OPEN_NEW_FOLDER;

	public bool changes_current_folder () {
		return this <= OPEN_NEW_FOLDER;
	}
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

public enum Gth.ZoomType {
	NATURAL_SIZE,
	KEEP_PREVIOUS,
	BEST_FIT,
	MAXIMIZE,
	MAXIMIZE_IF_LARGER,
	MAXIMIZE_WIDTH,
	MAXIMIZE_HEIGHT,
	FILL_SPACE;

	public bool fit_to_allocation () {
		return this >= BEST_FIT;
	}
}

public enum Gth.ZoomLimit {
	NONE,
	ALMOST_MAXIMIZE,
	MAXIMIZE,
}

public enum Gth.ZoomQuality {
	HIGH,
	LOW;
}

public enum Gth.TransparencyStyle {
	CHECKERED,
	WHITE,
	GRAY,
	BLACK;

	public unowned string get_state () {
		return STATE[this];
	}

	public void get_rgba (out Gdk.RGBA rgba) {
		if (this == WHITE) {
			rgba = { 1.0f, 1.0f, 1.0f, 1.0f };
		}
		else if (this == BLACK) {
			rgba = { 0.0f, 0.0f, 0.0f, 1.0f };
		}
		else {
			rgba = { 0.0f, 0.0f, 0.0f, 0.0f };
		}
	}

	const string[] STATE = {
		"checkered",
		"white",
		"gray",
		"black",
	};
}

[Flags]
public enum Gth.ViewFlags {
	DEFAULT,
	NO_DELAY,
	FULLSCREEN,
	FOCUS,
	WITH_NEW_POSITION,
	LOAD_PARENT_DIRECTORY,
}

public enum Gth.ScrollAction {
	CHANGE_FILE,
	CHANGE_ZOOM,
	CHANGE_VOLUME,
	CHANGE_CURRENT_TIME,
}

namespace Gth.PrivateAttribute {
	const string LOADED_IMAGE_IS_MODIFIED = "Loaded::Image::IsModified";
	const string LOADED_IMAGE_WAS_MODIFIED = "Loaded::Image::WasModified";
	const string LOADED_IMAGE_COLOR_PROFILE = "Loaded::Image::ColorProfile";
	const string ASK_FILENAME_WHEN_SAVING = "Loaded::Image::AskFilename";
}

[Flags]
public enum Gth.ForEachFlags {
	DEFAULT,
	RECURSIVE,
	NOFOLLOW_LINKS,
	READ_METADATA,
}

public enum Gth.ForEachAction {
	SKIP,
	CONTINUE,
	STOP;
}

public delegate Gth.ForEachAction Gth.ForEachChildFunc (Gth.FileData child, bool is_parent);

[Flags]
public enum Gth.TransformFlags {
	DEFAULT,
	CHANGE_IMAGE,
	RESET,
	ALWAYS_SAVE,
}

public enum Gth.GridType {
	NONE,
	RULE_OF_THIRDS,
	GOLDEN_RATIO,
	CENTER_LINES,
	UNIFORM;

	public unowned string to_state () {
		return STATE[this];
	}

	public static GridType from_state (string? state) {
		var idx = Util.enum_index (STATE, state);
		return (GridType) idx;
	}

	const string[] STATE = {
		"none", "rule-of-thirds", "golden-ratio", "center-lines", "uniform"
	};
}

public enum Gth.LoadState {
	NONE,
	LOADING,
	SUCCESS,
	ERROR,
}

public enum Gth.TextTransform {
	NONE,
	LOWERCASE,
	UPPERCASE;

	public unowned string to_state () {
		return STATE[this];
	}

	public static TextTransform from_state (string? state) {
		var idx = Util.enum_index (STATE, state);
		return (TextTransform) idx;
	}

	const string[] STATE = {
		"none", "lowercase", "uppercase"
	};
}

[Flags]
public enum Gth.QueryListFlags {
	DEFAULT = 0,
	NOT_RECURSIVE,
}

public enum Gth.ResizeUnit {
	PIXEL,
	PERCENT;

	public unowned string to_state () {
		return STATE[this];
	}

	public static ResizeUnit from_state (string? state) {
		var idx = Util.enum_index (STATE, state);
		return (ResizeUnit) idx;
	}

	const string[] STATE = { "pixel", "percent" };
}

public enum Gth.RotatedSize {
	ORIGINAL,
	BOUNDING_BOX,
	CROP_BORDERS;

	public unowned string to_state () {
		return STATE[this];
	}

	public static RotatedSize from_state (string? state) {
		var idx = Util.enum_index (STATE, state);
		return (RotatedSize) idx;
	}

	const string[] STATE = { "original", "bounding-box", "crop-borders" };
}

public enum Gth.Orientation {
	UP,
	RIGHT,
	DOWN,
	LEFT;

	public bool changes_dimensions () {
		return (this == RIGHT) || (this == LEFT);
	}

	public Transform to_transform () {
		return TRANSFORM[this];
	}

	public unowned string to_state () {
		return STATE[this];
	}

	public static Orientation from_state (string? state) {
		var idx = Util.enum_index (STATE, state);
		return (Orientation) idx;
	}

	const string[] STATE = {
		"up",
		"right",
		"down",
		"left",
	};

	const Transform[] TRANSFORM = {
		Transform.NONE,
		Transform.ROTATE_90,
		Transform.ROTATE_180,
		Transform.ROTATE_270,
	};
}

public enum Gth.PrintUnit {
	MM,
	INCH;

	public unowned string to_state () {
		return STATE[this];
	}

	public Gtk.Unit to_gtk_unit () {
		return UNIT[this];
	}

	public static PrintUnit from_state (string? state) {
		var idx = Util.enum_index (STATE, state);
		return (PrintUnit) idx;
	}

	public static double convertion_factor (PrintUnit from, PrintUnit to) {
		return CONVERTION_FACTOR[from, to];
	}

	const string[] STATE = {
		"mm",
		"in"
	};

	const Gtk.Unit[] UNIT = {
		Gtk.Unit.MM,
		Gtk.Unit.INCH,
	};

	const double[,] CONVERTION_FACTOR = {
		{ 1, 1 / 25.4 },
		{ 25.4, 1 },
	};
}
