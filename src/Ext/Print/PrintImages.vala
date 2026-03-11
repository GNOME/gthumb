public class Gth.PrintImages : Object {
	public async void print_files (Gth.MainWindow _window, GenericList<File> files, Job job) throws Error {
		window = _window;
		// Load the images
		var images = new GenericArray<PrintImageLayout> ();
		foreach (var file in files) {
			var image = yield app.image_loader.load_file (window.monitor_profile, file, LoadFlags.DEFAULT, job.cancellable);
			images.add (new PrintImageLayout (file, image));
		}
		print_layout = new PrintLayout (images);
		yield print (print_layout, job);
	}

	public async void print_image (Gth.MainWindow _window, Image image, File? file, Job job) throws Error {
		window = _window;
		var images = new GenericArray<PrintImageLayout> ();
		images.add (new PrintImageLayout (file, image));
		print_layout = new PrintLayout (images);
		yield print (print_layout, job);
	}

	async void print (PrintLayout print_layout, Job job) throws Error {
		try {
			while (true) {
				// Print Setup
				var print_dialog = new Gtk.PrintDialog ();
				yield ask_print_setup (print_dialog, job);

				// Layout Setup
				var result = yield ask_layout_setup (job);

				if (result == PrintDialogResult.PRINT) {
					yield exec_print (print_dialog, job);
					break;
				}

				if (result != PrintDialogResult.SETUP) {
					break;
				}
			}
		}
		catch (Error error) {
			throw error;
		}
		finally {
			print_layout.save_settings ();
		}
	}

	async void ask_print_setup (Gtk.PrintDialog print_dialog, Job job) throws Error {
		if (print_layout.page_setup != null) {
			print_dialog.page_setup = print_layout.page_setup;
		}
		if (print_layout.print_settings != null) {
			print_dialog.print_settings = print_layout.print_settings;
		}
		if (print_dialog.print_settings != null) {
			print_layout.set_default_output_file (print_dialog.print_settings);
		}
		print_dialog.accept_label = _("_Continue…");
		print_dialog.title = _("Print");
		try {
			job.opens_dialog ();
			var print_setup = yield print_dialog.setup (window, job.cancellable);
			print_layout.set_setup (print_setup);
		}
		catch (Error error) {
			throw error;
		}
		finally {
			job.dialog_closed ();
		}
	}

	async PrintDialogResult ask_layout_setup (Job job) throws Error {
		SourceFunc callback = ask_layout_setup.callback;
		var layout_dialog = new PrintLayoutDialog (window, print_layout);
		layout_dialog.close_request.connect (() => {
			if (callback != null) {
				Idle.add ((owned) callback);
				callback = null;
			}
			return false;
		});
		var cancelled_event = job.cancellable.cancelled.connect (() => {
			layout_dialog.close ();
		});
		job.opens_dialog ();
		layout_dialog.transient_for = window;
		layout_dialog.present ();
		yield;
		job.dialog_closed ();
		if (cancelled_event != 0) {
			job.cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		return layout_dialog.result;
	}

	async void exec_print (Gtk.PrintDialog print_dialog, Job job) throws Error {
		try {
			job.opens_dialog ();
			var stream = yield print_dialog.print (window, print_layout.print_setup, job.cancellable);
			print_layout.print_to_stream (stream, job.cancellable);
			stream.close (job.cancellable);
		}
		catch (Error error) {
			throw error;
		}
		finally {
			job.dialog_closed ();
		}
	}

	Gth.MainWindow window;
	PrintLayout print_layout;
}
