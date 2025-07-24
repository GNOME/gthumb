public interface Gth.PropertyView : Gtk.Widget {
	public abstract unowned string get_name ();
	public abstract unowned string get_title ();
	public abstract unowned string get_icon ();
	public abstract bool can_view (Gth.FileData file_data);
	public abstract void set_file (Gth.FileData? file_data);
	public abstract bool with_search ();
	public abstract void set_search (string? text);
}
