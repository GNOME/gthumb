public class Gth.JxlPreferences : Gth.SaverPreferences {
	public override unowned string get_content_type () {
		return "image/jxl";
	}

	public override unowned string get_display_name () {
		// Translators: file type
		return _("JXL");
	}

	public override string get_default_extension () {
		return "jxl";
	}

	public override unowned string get_extensions () {
		return "jxl";
	}

	public override bool can_save_icc_profile () {
		return true;
	}

	public override Adw.PreferencesPage create_widget (bool only_format_options = false) {
		if (builder == null) {
			builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/jxl-preferences.ui");
		}

		unowned var adj = builder.get_object ("effort_adjustment") as Gtk.Adjustment;
		adj.set_value (settings.get_int (PREF_JXL_EFFORT));
		adj.value_changed.connect ((local_adj) => {
			settings.set_int (PREF_JXL_EFFORT, (int) local_adj.get_value ());
		});

		adj = builder.get_object ("distance_adjustment") as Gtk.Adjustment;
		adj.set_value (settings.get_double (PREF_JXL_DISTANCE));
		adj.value_changed.connect ((local_adj) => {
			settings.set_double (PREF_JXL_DISTANCE, local_adj.get_value ());
		});

		unowned var switch_row = builder.get_object ("lossless") as Adw.SwitchRow;

		switch_row.notify["active"].connect ((obj, param) => {
			unowned var local_switch_row = obj as Adw.SwitchRow;
			unowned var local_distance_row = builder.get_object ("distance_row") as Adw.ActionRow;
			local_distance_row.sensitive = !local_switch_row.active;
		});

		switch_row.active = settings.get_boolean (PREF_JXL_LOSSLESS);
		switch_row.notify["active"].connect ((obj, param) => {
			unowned var row = obj as Adw.SwitchRow;
			settings.set_boolean (PREF_JXL_LOSSLESS, row.active);
		});

		return builder.get_object ("page") as Adw.PreferencesPage;
	}

	public override Gth.Option[] get_options () {
		Gth.Option[] options = {};
		options += new Gth.Option.int (JxlOption.EFFORT, settings.get_int (PREF_JXL_EFFORT));
		options += new Gth.Option.double (JxlOption.DISTANCE, settings.get_double (PREF_JXL_DISTANCE));
		options += new Gth.Option.bool (JxlOption.LOSSLESS, settings.get_boolean (PREF_JXL_LOSSLESS));
		options += null;
		return options;
	}

	construct {
		settings = new GLib.Settings (GTHUMB_JXL_SAVER_SCHEMA);
		builder = null;
	}

	Gtk.Builder builder;
	GLib.Settings settings;
}
