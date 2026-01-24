public abstract class Gth.SaverPreferences : Object {
	public abstract unowned string get_content_type ();
	public abstract unowned string get_display_name ();
	public abstract string get_default_extension ();
	public abstract unowned string get_extensions ();
	public abstract bool can_save_icc_profile ();
	public abstract Adw.PreferencesPage create_widget (bool only_format_options = false);
	public abstract Gth.Option[] get_options ();
}
