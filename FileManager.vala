public class Gth.FileManager {
	public FileManager (Window _window) {
		window = _window;
	}

	public async void delete_files (GenericList<FileData> files, Job job) throws Error {
		// Ask confirmation
		var dialog = new Adw.AlertDialog (_("Delete Permanently?"), null);
		dialog.body_use_markup = true;
		if (files.model.n_items == 1) {
			var file = files.first ();
			dialog.body = _("The file <i>%s</i> will be permanently lost. Do you want to delete it?").printf (file.info.get_display_name ());
		}
		else {
			dialog.body = GLib.ngettext (
				"The %'d selected file will be permanently lost. Do you want to delete it?",
				"The %'d selected files will be permanently lost. Do you want to delete them?",
				files.model.n_items
			).printf (files.model.n_items);
		}
		dialog.add_responses (
			"cancel",  _("_Cancel"),
			"delete", _("_Delete")
		);
		dialog.set_response_appearance ("delete", Adw.ResponseAppearance.DESTRUCTIVE);
		dialog.default_response = "cancel";
		dialog.close_response = "cancel";
		var response = yield dialog.choose (window, job.cancellable);
		if (response == "cancel") {
			throw new IOError.CANCELLED ("Cancelled");
		}

		// Delete
		var iter = files.iterator ();
		var deleted_files = new GenericList<File>();
		while (iter.next ()) {
			var file = iter.get ().file;
			yield file.delete_async (Priority.DEFAULT, job.cancellable);
			job.progress = (float) iter.index () / files.model.n_items;
			deleted_files.model.append (file);
		}
		app.monitor.files_deleted (deleted_files);
	}

	public async void trash_files (GenericList<FileData> files, Job job) throws Error {
		var iter = files.iterator ();
		var deleted_files = new GenericList<File>();
		while (iter.next ()) {
			var file = iter.get ().file;
			yield file.trash_async (Priority.DEFAULT, job.cancellable);
			job.progress = (float) iter.index () / files.model.n_items;
			deleted_files.model.append (file);
		}
		app.monitor.files_deleted (deleted_files);
	}

	weak Window window;
}
