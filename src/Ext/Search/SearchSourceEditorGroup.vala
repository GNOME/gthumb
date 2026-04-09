[GtkTemplate (ui = "/org/gnome/gthumb/ui/search-source-editor-group.ui")]
public class Gth.SearchSourceEditorGroup : Adw.PreferencesGroup {
	public GenericList<SearchSource> sources;
	public File default_folder;

	construct {
		var empty_row = new Adw.ActionRow ();
		empty_row.title = _("Empty");
		empty_row.sensitive = false;
		empty_row.halign = Gtk.Align.CENTER;
		source_list.set_placeholder (empty_row);

		default_folder = Files.get_home ();
	}

	public void set_sources (GenericList<SearchSource>? _sources) {
		if (_sources != null) {
			sources = _sources;
		}
		else {
			sources = new GenericList<SearchSource> ();
		}
		source_list.bind_model (sources.model, new_source_row);
	}

	Gtk.Widget new_source_row (Object item) {
		var file_source = item as Gth.SearchSource;

		var row = new Gth.SearchSourceRow (file_source);
		row.move_to_row.connect ((source_row, target_row) => {
			var source_pos = source_row.get_index ();
			var target_pos = target_row.get_index ();
			if ((target_pos >= 0) && (target_pos != source_pos)) {
				sources.model.remove (source_pos);
				sources.model.insert (target_pos, source_row.search_source);
			}
		});
		row.delete_row.connect ((source_row) => {
			if (sources.length () > 1) {
				sources.model.remove (source_row.get_index ());
			}
		});

		return row;
	}

	[GtkCallback]
	private void on_add_source (Gtk.Button source) {
		var selector = new Gth.FolderSelector ();
		selector.select_folder.begin (get_root () as Gth.MainWindow, default_folder, null, (obj, _res) => {
			try {
				var folder = selector.select_folder.end (_res);
				sources.model.append (new SearchSource (folder, true));
			}
			catch (Error error) {
			}
		});
	}

	[GtkChild] unowned Gtk.ListBox source_list;
}
