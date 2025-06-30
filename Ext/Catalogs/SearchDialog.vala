[GtkTemplate (ui = "/app/gthumb/gthumb/ui/search-dialog.ui")]
public class Gth.SearchDialog : Adw.ApplicationWindow {
	public SearchDialog (File _default_folder, CatalogSearch? _search = null) {
		sources_group.default_folder = _default_folder;
		set_search (_search);
	}

	public void set_search (CatalogSearch? _search) {
		if (_search != null) {
			search = _search;
		}
		else {
			search = new CatalogSearch ();
			search.test = new TestExpr (TestExpr.Operation.INTERSECTION);
			search.test.add (new Gth.TestFileName ());
			search.sources.model.append (new SearchSource (sources_group.default_folder, true));
		}
		rules_group.set_expr (search.test);
		sources_group.set_sources (search.sources);
	}

	public CatalogSearch? get_search () throws Error {
		rules_group.update_from_options ();
		return search;
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

	[GtkChild] unowned Gth.TestExprEditorGroup rules_group;
	[GtkChild] unowned Gth.SearchSourceEditorGroup sources_group;

	CatalogSearch search;
}
