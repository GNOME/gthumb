public class Gth.SearchEditor : Object {
	public async Gth.Catalog? new_search (Gtk.Window? parent, File default_folder, Job job) throws Error {
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
		cancelled_event = job.cancellable.cancelled.connect (() => {
			dialog.close ();
		});
		job.opens_dialog ();
		dialog.present (parent);
		dialog.focus_first_rule ();
		yield;
		job.dialog_closed ();
		if (cancelled_event != 0) {
			job.cancellable.disconnect (cancelled_event);
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
	private void on_save (Gtk.Button source) {
		try {
			rules_group.update_from_options ();
			changed ();
		}
		catch (Error error) {
			toast_overlay.dismiss_all ();
			toast_overlay.add_toast (Util.new_literal_toast (error.message));
		}
	}

	[GtkChild] unowned Gth.TestExprEditorGroup rules_group;
	[GtkChild] unowned Gth.SearchSourceEditorGroup sources_group;
	[GtkChild] unowned Adw.ToastOverlay toast_overlay;

	CatalogSearch catalog;
}
