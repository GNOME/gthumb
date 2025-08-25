public class Gth.JpegPreferences : Gth.SaverPreferences {
	public override unowned string get_display_name () {
		// Translators: file type
		return _("JPEG");
	}

	public override string get_default_extension () {
		return settings.get_string (PREF_JPEG_DEFAULT_EXT);
	}

	public override unowned string get_extensions () {
		return "jpeg,jpg";
	}

	public override bool can_save_icc_profile () {
		return false;
	}

	public override Adw.PreferencesPage create_widget (bool only_format_options = false) {
		if (builder == null) {
			builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/jpeg-preferences.ui");
		}

		unowned var combo_row = builder.get_object ("default_extension") as Adw.ComboRow;
		combo_row.selected = settings.get_string (PREF_JPEG_DEFAULT_EXT) == "jpeg" ? 0 : 1;
		combo_row.notify["selected"].connect ((obj, param) => {
			unowned var row = obj as Adw.ComboRow;
			settings.set_string (PREF_JPEG_DEFAULT_EXT, EXTENSIONS[row.selected]);
		});

		unowned var adj = builder.get_object ("quality_adjustment") as Gtk.Adjustment;
		adj.set_value (settings.get_int (PREF_JPEG_QUALITY));
		adj.value_changed.connect ((local_adj) => {
			settings.set_int (PREF_JPEG_QUALITY, (int) local_adj.get_value ());
		});

		adj = builder.get_object ("smoothing_adjustment") as Gtk.Adjustment;
		adj.set_value (settings.get_int (PREF_JPEG_SMOOTHING));
		adj.value_changed.connect ((local_adj) => {
			settings.set_int (PREF_JPEG_SMOOTHING, (int) local_adj.get_value ());
		});

		unowned var switch_row = builder.get_object ("optimize") as Adw.SwitchRow;
		switch_row.active = settings.get_boolean (PREF_JPEG_OPTIMIZE);
		switch_row.notify["active"].connect ((obj, param) => {
			unowned var row = obj as Adw.SwitchRow;
			settings.set_boolean (PREF_JPEG_OPTIMIZE, row.active);
		});

		switch_row = builder.get_object ("progressive") as Adw.SwitchRow;
		switch_row.active = settings.get_boolean (PREF_JPEG_PROGRESSIVE);
		switch_row.notify["active"].connect ((obj, param) => {
			unowned var row = obj as Adw.SwitchRow;
			settings.set_boolean (PREF_JPEG_PROGRESSIVE, row.active);
		});

		if (only_format_options) {
			var group = builder.get_object ("other_preferences") as Gtk.Widget;
			group.visible = false;
		}

		return builder.get_object ("page") as Adw.PreferencesPage;
	}

	public override Gth.Option[] get_options () {
		Gth.Option[] options = {};
		options += new Gth.Option.int (JpegOption.QUALITY, settings.get_int (PREF_JPEG_QUALITY));
		options += new Gth.Option.int (JpegOption.SMOOTHING, settings.get_int (PREF_JPEG_SMOOTHING));
		options += new Gth.Option.bool (JpegOption.OPTIMIZE, settings.get_boolean (PREF_JPEG_OPTIMIZE));
		options += new Gth.Option.bool (JpegOption.PROGRESSIVE, settings.get_boolean (PREF_JPEG_PROGRESSIVE));
		options += null;
		return options;
	}

	construct {
		settings = new GLib.Settings (GTHUMB_JPEG_SAVER_SCHEMA);
		builder = null;
	}

	Gtk.Builder builder;
	GLib.Settings settings;

	const string[] EXTENSIONS = { "jpeg", "jpg" };
}
