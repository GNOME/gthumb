[GtkTemplate (ui = "/app/gthumb/gthumb/ui/search-dialog.ui")]
public class Gth.SearchDialog : Adw.ApplicationWindow {
	public SearchDialog (File _default_folder) {
		default_folder = _default_folder;
		var expr = new TestExpr (TestExpr.Operation.INTERSECTION);
		expr.add (new Gth.TestFileName ());
		rules_group.set_expr (expr);

		// Source list
		sources = new GenericList<SearchSource> ();
		sources.model.append (new SearchSource (default_folder, true));
		source_list.bind_model (sources.model, new_source_row);

		var empty_row = new Adw.ActionRow ();
		empty_row.title = _("No Folder");
		empty_row.sensitive = false;
		empty_row.halign = Gtk.Align.CENTER;
		source_list.set_placeholder (empty_row);
	}

	public void focus_first_rule () {
		rules_group.expr.focus_options ();
	}

	[GtkCallback]
	private void on_cancel (Gtk.Button source) {
		close ();
	}

	[GtkCallback]
	private void on_search (Gtk.Button source) {
	}

	[GtkCallback]
	private void on_add_source (Gtk.Button source) {
		var selector = new Gth.FolderSelector ();
		selector.select_folder.begin (get_root () as Gtk.Window, default_folder, null, (obj, _res) => {
			try {
				var folder = selector.select_folder.end (_res);
				sources.model.append (new SearchSource (folder, true));
			}
			catch (Error error) {
			}
		});
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
			sources.model.remove (source_row.get_index ());
		});

		return row;
	}

	[GtkChild] unowned Gth.TestExprEditorGroup rules_group;
	[GtkChild] unowned Gtk.ListBox source_list;

	GenericList<SearchSource> sources;
	File default_folder;
}
