[GtkTemplate (ui = "/app/gthumb/gthumb/ui/browser-preferences.ui")]
public class Gth.BrowserPreferences : Adw.NavigationPage {
	construct {
		settings = new GLib.Settings (GTHUMB_SCHEMA);
		constructing = true;
		thumbnail_size_adjustment.set_value (settings.get_int (PREF_BROWSER_THUMBNAIL_SIZE));

		// Caption
		caption_model.append (_("Hidden"));
		foreach (unowned var attr_id in CAPTION_ATTRIBUTES) {
			unowned var info = MetadataInfo.get (attr_id);
			if (info != null) {
				caption_model.append (info.display_name);
			}
		}
		var caption_s = settings.get_string (PREF_BROWSER_THUMBNAIL_CAPTION);
		var caption_v = app.migration.metadata.get_new_key_v (caption_s);
		var row = 0;
		foreach (unowned var caption in caption_v) {
			var idx = get_caption_index (caption);
			if (idx != uint.MAX) {
				if (row == 0) {
					caption_row_1.selected = idx + 1;
				}
				if (row == 1) {
					caption_row_2.selected = idx + 1;
				}
				if (row == 2) {
					caption_row_3.selected = idx + 1;
				}
				row++;
				if (row == 3) {
					break;
				}
			}
		}
		constructing = false;
	}

	uint get_caption_index (string id) {
		uint idx = 0;
		foreach (unowned var attr_id in CAPTION_ATTRIBUTES) {
			if (id == attr_id) {
				return idx;
			}
			idx++;
		}
		return uint.MAX;
	}

	[GtkCallback]
	void on_thumbnail_size_changed (Gtk.Adjustment adj) {
		if (constructing) {
			return;
		}
		settings.set_int (PREF_BROWSER_THUMBNAIL_SIZE, (int) adj.get_value ());
	}

	[GtkCallback]
	bool on_thumbnail_size_change_value (Gtk.Range range, Gtk.ScrollType scroll, double new_value) {
		if (constructing) {
			return false;
		}
		var int_value = (int) (Math.round (new_value / THUMBNAIL_SIZE_STEP) * THUMBNAIL_SIZE_STEP);
		range.set_value (int_value);
		return true;
	}

	[GtkCallback]
	void on_caption_selected (Object row, ParamSpec param) {
		if (constructing) {
			return;
		}
		var caption_v = new StringBuilder ();
		if (caption_row_1.selected > 0) {
			unowned var attr = CAPTION_ATTRIBUTES[caption_row_1.selected - 1];
			caption_v.append (app.migration.metadata.get_old_key (attr));
		}
		if (caption_row_2.selected > 0) {
			if (caption_v.len > 0) {
				caption_v.append (",");
			}
			unowned var attr = CAPTION_ATTRIBUTES[caption_row_2.selected - 1];
			caption_v.append (app.migration.metadata.get_old_key (attr));
		}
		if (caption_row_3.selected > 0) {
			if (caption_v.len > 0) {
				caption_v.append (",");
			}
			unowned var attr = CAPTION_ATTRIBUTES[caption_row_3.selected - 1];
			caption_v.append (app.migration.metadata.get_old_key (attr));
		}
		settings.set_string (PREF_BROWSER_THUMBNAIL_CAPTION, caption_v.str);
	}

	[GtkChild] unowned Gtk.Adjustment thumbnail_size_adjustment;
	[GtkChild] unowned Adw.ComboRow caption_row_1;
	[GtkChild] unowned Adw.ComboRow caption_row_2;
	[GtkChild] unowned Adw.ComboRow caption_row_3;
	[GtkChild] unowned Gtk.StringList caption_model;

	GLib.Settings settings;
	bool constructing;

	const double THUMBNAIL_SIZE_STEP = 32.0;
	const string[] CAPTION_ATTRIBUTES = {
		"standard::display-name",
		"Private::File::DisplaySize",
		"Private::File::DisplayCreated",
		"Private::File::DisplayModified",
		"Frame::Pixels",
		"Metadata::Title",
		"Metadata::Rating",
	};
}
