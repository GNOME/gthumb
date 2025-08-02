public abstract class Gth.SaverPreferences : Object {
	public abstract unowned string get_display_name ();
	public abstract string get_default_extension ();
	public abstract unowned string get_extensions ();
	public abstract bool can_save_icc_profile ();
	public abstract Gtk.Widget? create_widget ();
	public abstract Gth.Option[] get_options ();
}
