public class Gth.TiffPreferences : Gth.SaverPreferences {
	public override unowned string get_content_type () {
		return "image/tiff";
	}

	public override unowned string get_display_name () {
		// Translators: file type
		return _("TIFF");
	}

	public override string get_default_extension () {
		return settings.get_string (PREF_TIFF_DEFAULT_EXT);
	}

	public override unowned string get_extensions () {
		return "tiff,tif";
	}

	public override bool can_save_icc_profile () {
		return true;
	}

	public override Adw.PreferencesPage create_widget (bool only_format_options = false) {
		if (builder == null) {
			builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/tiff-preferences.ui");
		}

		unowned var combo_row = builder.get_object ("default_extension") as Adw.ComboRow;
		combo_row.selected = settings.get_string (PREF_TIFF_DEFAULT_EXT) == "tiff" ? 0 : 1;
		combo_row.notify["selected"].connect ((obj, param) => {
			unowned var row = obj as Adw.ComboRow;
			settings.set_string (PREF_TIFF_DEFAULT_EXT, EXTENSIONS[row.selected]);
		});

		combo_row = builder.get_object ("compression") as Adw.ComboRow;
		combo_row.selected = settings.get_enum (PREF_TIFF_COMPRESSION);
		combo_row.notify["selected"].connect ((obj, param) => {
			unowned var row = obj as Adw.ComboRow;
			settings.set_string (PREF_TIFF_DEFAULT_EXT, COMPRESSIONS[row.selected]);
		});

		unowned var adj = builder.get_object ("hresolution") as Gtk.Adjustment;
		adj.set_value (settings.get_int (PREF_TIFF_HRESOLUTION));
		adj.value_changed.connect ((local_adj) => {
			settings.set_int (PREF_TIFF_HRESOLUTION, (int) local_adj.get_value ());
		});

		adj = builder.get_object ("vresolution") as Gtk.Adjustment;
		adj.set_value (settings.get_int (PREF_TIFF_VRESOLUTION));
		adj.value_changed.connect ((local_adj) => {
			settings.set_int (PREF_TIFF_VRESOLUTION, (int) local_adj.get_value ());
		});

		if (only_format_options) {
			var group = builder.get_object ("other_preferences") as Gtk.Widget;
			group.visible = false;
		}

		return builder.get_object ("page") as Adw.PreferencesPage;
	}

	public override Gth.Option[] get_options () {
		Gth.Option[] options = {};
		options += new Gth.Option.enum (TiffOption.COMPRESSION, settings.get_enum (PREF_TIFF_COMPRESSION));
		options += new Gth.Option.int (TiffOption.HRESOLUTION, (int) settings.get_int (PREF_TIFF_HRESOLUTION));
		options += new Gth.Option.int (TiffOption.VRESOLUTION, (int) settings.get_int (PREF_TIFF_VRESOLUTION));
		options += null;
		return options;
	}

	construct {
		settings = new GLib.Settings (GTHUMB_TIFF_SAVER_SCHEMA);
		builder = null;
	}

	Gtk.Builder builder;
	GLib.Settings settings;

	const string[] EXTENSIONS = { "tiff", "tif" };
	const string[] COMPRESSIONS = { "none", "lossless", "jpeg" };
}
