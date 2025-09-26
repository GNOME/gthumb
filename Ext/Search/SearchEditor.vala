public class Gth.SearchEditor : Object {
	public async Gth.Catalog? new_search (Gtk.Window? parent, File default_folder, Cancellable? cancellable = null) throws Error {
		callback = new_search.callback;
		dialog = new SearchDialog (default_folder);
		dialog.changed.connect (() => {
			result = dialog.get_catalog ();
			dialog.close ();
		});
		dialog.closed.connect (() => {
			if (callback != null) {
				Idle.add ((owned) callback);
				callback = null;
			}
		});
		cancelled_event = cancellable.cancelled.connect (() => {
			dialog.close ();
		});
		dialog.present (parent);
		yield;
		if (cancelled_event != 0) {
			cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (result == null) {
			throw new IOError.CANCELLED ("Cancelled");
		}
		return result;
	}

	SourceFunc callback = null;
	SearchDialog dialog = null;
	ulong cancelled_event = 0;
	Catalog? result = null;
}


[GtkTemplate (ui = "/app/gthumb/gthumb/ui/search-dialog.ui")]
class Gth.SearchDialog : Adw.Dialog {
	public signal void changed ();

	public SearchDialog (File default_folder) {
		sources_group.default_folder = default_folder;

		catalog = new CatalogSearch ();
		catalog.name = _("Last Search");
		catalog.file = Catalog.from_display_name (catalog.name, ".search");
		catalog.test = new TestExpr (TestExpr.Operation.INTERSECTION);
		catalog.test.add (new Gth.TestFileName ());
		catalog.sources.model.append (new SearchSource (sources_group.default_folder, true));

		rules_group.set_expr (catalog.test);
		sources_group.set_sources (catalog.sources);
	}

	public Catalog? get_catalog () {
		return catalog;
	}

	public void focus_first_rule () {
		rules_group.expr.focus_options ();
	}

	[GtkCallback]
	void on_cancel (Gtk.Button source) {
		close ();
	}

	[GtkCallback]
	private void on_save (Gtk.Button source) {
		try {
			rules_group.update_from_options ();
			changed ();
		}
		catch (Error error) {
			// TODO: show error
		}
	}

	[GtkChild] unowned Gth.TestExprEditorGroup rules_group;
	[GtkChild] unowned Gth.SearchSourceEditorGroup sources_group;

	CatalogSearch catalog;
}
