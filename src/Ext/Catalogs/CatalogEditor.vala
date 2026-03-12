public class Gth.CatalogEditor : Object {
	public async Gth.Catalog? edit_catalog (Gtk.Window? parent, File? file = null, Job job) throws Error {
		callback = edit_catalog.callback;
		dialog = new CatalogDialog ();
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
		yield dialog.load_file (file, job.cancellable);
		job.opens_dialog ();
		dialog.present (parent);
		yield;
		job.dialog_closed ();
		if (cancelled_event != 0) {
			job.cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (result == null) {
			throw new IOError.CANCELLED ("Cancelled");
		}
		original = dialog.get_original ();
		return result;
	}

	public bool search_parameters_changed () {
		if ((result == null) || (original == null)) {
			return false;
		}
		if ((result is CatalogSearch) && (original is CatalogSearch)) {
			var result_s = result as CatalogSearch;
			var original_s = original as CatalogSearch;
			return !result_s.equal_search_parameters (original_s);
		}
		return false;
	}

	SourceFunc callback = null;
	CatalogDialog dialog = null;
	ulong cancelled_event = 0;
	Catalog? result = null;
	Catalog? original = null;
}


[GtkTemplate (ui = "/app/gthumb/gthumb/ui/catalog-dialog.ui")]
class Gth.CatalogDialog : Adw.Dialog {
	public signal void changed ();

	public async void load_file (File file, Cancellable cancellable) throws Error {
		var gio_file = Catalog.to_gio_file (file);
		var data = yield Files.load_contents_async (gio_file, cancellable);
		var catalog = Catalog.new_from_data (file, data);
		set_catalog (catalog);
	}

	public Catalog get_catalog () {
		return catalog;
	}

	public Catalog get_original () {
		return original_catalog;
	}

	[GtkCallback]
	void on_cancel (Gtk.Button source) {
		close ();
	}

	[GtkCallback]
	private void on_save (Gtk.Button source) {
		try {
			catalog.name = name_row.get_text ();
			catalog.date = date_row.date;
			if (catalog is CatalogSearch) {
				rules_group.update_from_options ();
			}
			changed ();
		}
		catch (Error error) {
			toast_overlay.dismiss_all ();
			toast_overlay.add_toast (Util.new_literal_toast (error.message));
		}
	}

	void set_catalog (Catalog _catalog) {
		original_catalog = _catalog;
		catalog = original_catalog.duplicate ();
		if (catalog is CatalogSearch) {
			var search = catalog as CatalogSearch;
			rules_group.set_expr (search.test);
			sources_group.set_sources (search.sources);
			sources_group.visible = true;
			rules_group.visible = true;
			// Translators: noun (not verb)
			set_title (C_("Noun", "Search"));
		}
		else {
			sources_group.visible = false;
			rules_group.visible = false;
		}
		name_row.text = catalog.name;
		date_row.date = catalog.date;
	}

	[GtkChild] unowned Adw.EntryRow name_row;
	[GtkChild] unowned Gth.DateRow date_row;
	[GtkChild] unowned Gth.SearchSourceEditorGroup sources_group;
	[GtkChild] unowned Gth.TestExprEditorGroup rules_group;
	[GtkChild] unowned Adw.ToastOverlay toast_overlay;

	Catalog original_catalog;
	Catalog catalog;
}
