public class Gth.Widgets {
	public static Gtk.ListBox new_boxed_list () {
		var list_box = new Gtk.ListBox ();
		list_box.add_css_class ("boxed-list");
		list_box.selection_mode = Gtk.SelectionMode.NONE;
		return list_box;
	}
}
