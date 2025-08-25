public class Gth.PngPreferences : Gth.SaverPreferences {
	public override unowned string get_display_name () {
		// Translators: file type
		return _("PNG");
	}

	public override string get_default_extension () {
		return "png";
	}

	public override unowned string get_extensions () {
		return "png";
	}

	public override bool can_save_icc_profile () {
		return true;
	}

	public override Adw.PreferencesPage create_widget (bool only_format_options = false) {
		if (builder == null) {
			builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/png-preferences.ui");
		}

		unowned var adj = builder.get_object ("compression_adjustment") as Gtk.Adjustment;
		adj.set_value (settings.get_int (PREF_PNG_COMPRESSION_LEVEL));
		adj.value_changed.connect ((local_adj) => {
			settings.set_int (PREF_PNG_COMPRESSION_LEVEL, (int) local_adj.get_value ());
		});

		return builder.get_object ("page") as Adw.PreferencesPage;
	}

	public override Gth.Option[] get_options () {
		Gth.Option[] options = {};
		options += new Gth.Option.int (PngOption.COMPRESSION_LEVEL, settings.get_int (PREF_PNG_COMPRESSION_LEVEL));
		options += null;
		return options;
	}

	construct {
		settings = new GLib.Settings (GTHUMB_PNG_SAVER_SCHEMA);
		builder = null;
	}

	Gtk.Builder builder;
	GLib.Settings settings;
}
