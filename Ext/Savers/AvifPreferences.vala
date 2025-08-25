public class Gth.AvifPreferences : Gth.SaverPreferences {
	public override unowned string get_display_name () {
		// Translators: file type
		return _("AVIF");
	}

	public override string get_default_extension () {
		return "avif";
	}

	public override unowned string get_extensions () {
		return "avif";
	}

	public override bool can_save_icc_profile () {
		return false;
	}

	public override Adw.PreferencesPage create_widget (bool only_format_options = false) {
		if (builder == null) {
			builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/avif-preferences.ui");
		}

		unowned var adj = builder.get_object ("quality_adjustment") as Gtk.Adjustment;
		adj.set_value (settings.get_int (PREF_AVIF_QUALITY));
		adj.value_changed.connect ((local_adj) => {
			settings.set_int (PREF_AVIF_QUALITY, (int) local_adj.get_value ());
		});

		unowned var switch_row = builder.get_object ("lossless") as Adw.SwitchRow;
		switch_row.active = settings.get_boolean (PREF_AVIF_LOSSLESS);
		switch_row.notify["active"].connect ((obj, param) => {
			unowned var row = obj as Adw.SwitchRow;
			settings.set_boolean (PREF_AVIF_LOSSLESS, row.active);
		});

		return builder.get_object ("page") as Adw.PreferencesPage;
	}

	public override Gth.Option[] get_options () {
		Gth.Option[] options = {};
		options += new Gth.Option.int (AvifOption.QUALITY, settings.get_int (PREF_AVIF_QUALITY));
		options += new Gth.Option.bool (AvifOption.LOSSLESS, settings.get_boolean (PREF_AVIF_LOSSLESS));
		options += null;
		return options;
	}

	construct {
		settings = new GLib.Settings (GTHUMB_AVIF_SAVER_SCHEMA);
		builder = null;
	}

	Gtk.Builder builder;
	GLib.Settings settings;
}
