const int HORIZONTAL_SPACING = 12; // pixels
const int VERTICAL_SPACING = 12; // pixels
const int CONTAINER_H_PADDING = 12; // pixels
const int CONTAINER_V_PADDING = 12; // pixels
const string FILTERS_FILE = "filters.xml";

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

public struct Gth.MetadataInfo {
	string id;
	string display_name;
	string category;
	int sort_order;
	string type;
	Gth.MetadataFlags flags;
}

[Flags]
public enum Gth.MetadataFlags {
	ALLOW_NOWHERE,
	ALLOW_IN_FILE_LIST,
	ALLOW_IN_PROPERTIES_VIEW,
	ALLOW_IN_PRINT,
}

public delegate void Gth.IdleFunc ();
