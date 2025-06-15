using GLib;

[CCode (cheader_filename = "lib/gth-metadata.h", has_type_id = false)]
public struct Gth.MetadataInfo {
	public string id;
	public string display_name;
	public string category;
	public int sort_order;
	public string type;
	public MetadataFlags flags;

	public static void init ();
	public static void register (string id, string display_name, string category, MetadataFlags flags, string? type = null);
	[CCode ()]
	public static unowned MetadataInfo? get (string id);
	public static unowned GenericArray<MetadataInfo?> get_all ();
}

[Compact]
[CCode (cheader_filename = "lib/gth-metadata.h", has_type_id = false)]
public class Gth.MetadataCategory {
	public string id;
	public string display_name;
	public int sort_order;

	public MetadataCategory (string id, string display_name, int sort_order);
	public static void init ();
	public static void register (string id, string display_name);
	public static unowned MetadataCategory get (string id);
}

[CCode (cheader_filename = "lib/gth-metadata.h", cprefix = "GTH_METADATA_", has_type_id = false)]
[Flags]
public enum Gth.MetadataFlags {
	HIDDEN,
	ALLOW_IN_FILE_LIST,
	ALLOW_IN_PROPERTIES_VIEW,
	ALLOW_IN_PRINT,
}

[CCode (cheader_filename = "lib/gth-metadata.h")]
public enum Gth.MetadataType {
	STRING,
	STRING_LIST
}


[CCode (cheader_filename = "lib/gth-metadata.h")]
public class Gth.Metadata : Object {
	public Metadata ();
	public Metadata.from_string_list (StringList list);
	public MetadataType get_data_type ();
	public unowned string get_id ();
	public unowned string get_raw ();
	public unowned StringList get_string_list ();
	public unowned string get_formatted ();
	public unowned string get_value_type ();
	public Metadata dup ();
}
