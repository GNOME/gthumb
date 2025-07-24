[Compact]
[CCode (
	cname = "GthOption",
	lower_case_cprefix = "gth_option_",
	cheader_filename = "lib/gth-option.h",
	ref_function = "gth_option_ref",
	unref_function = "gth_option_unref",
	type_id = "GTH_TYPE_OPTION")]
public class Gth.Option {
	public Option.void (int id);
	public Option.bool (int id, bool value);
	public Option.int (int id, int value);
	public Option.uint (int id, uint value);
	public Option.enum (int id, int value);
	public Option.flags (int id, int value);
	public Option.intv ([CCode (array_length_cname = "len", array_length_pos = 2.1)] int id, int[] value);
	public Option.uintv ([CCode (array_length_cname = "len", array_length_pos = 2.1)] int id, uint[] value);
	public Option.string ([CCode (array_length_cname = "len", array_length_pos = 2.1)] int id, string value);

	public int get_id ();
	public bool get_bool (ref bool value);
	public bool get_int (ref int value);
	public bool get_uint (ref uint value);
	public bool get_enum (ref int value);
	public bool get_flags (ref int value);
	public bool get_intv ([CCode (array_length_cname = "len", array_length_pos = 2.1)] ref int[] value);
	public bool get_uintv ([CCode (array_length_cname = "len", array_length_pos = 2.1)] ref uint[] value);
	public bool get_string ([CCode (array_length_cname = "len", array_length_pos = 2.1)] ref string value);

	public bool equal (Option other);
}
