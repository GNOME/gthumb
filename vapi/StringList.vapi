using GLib;

[CCode (cheader_filename = "lib/gth-string-list.h")]
public class Gth.StringList : Object {
	public StringList (List list);
	public StringList.from_strv (string[] strv);
	[CCode (cname = "gth_string_list_new_from_ptr_array")]
	public StringList.from_array (GenericArray<string> array);
	public unowned List get_list ();
	public void set_list (List list);
	public string join (string separator);
	public bool equal (StringList other);
	public bool equal_custom (StringList other, CompareFunc cmp_func);
	public void append (StringList other);
	public void concat (StringList other);
}
