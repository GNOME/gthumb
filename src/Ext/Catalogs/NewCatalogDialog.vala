public class Gth.NewCatalogDialog : Object {
	public async File new_catalog (File parent, Gtk.Window window, Job job) throws Error {
		var read_filename = new ReadFilename (_("New Catalog"), _("_Create"));
		var basename = yield read_filename.read_value (window, job);
		var catalog = new Catalog ();
		catalog.name = basename;
		catalog.file = parent.get_child_for_display_name (basename + ".catalog");
		yield catalog.save_async (job.cancellable);
		return catalog.file;
	}

	public async File new_library (File parent, Gtk.Window window, Job job) throws Error {
		var read_filename = new ReadFilename (_("New Library"), _("_Create"));
		var basename = yield read_filename.read_value (window, job);
		var folder = parent.get_child_for_display_name (basename);
		var gio_folder = Catalog.to_gio_file (folder);
		yield Files.make_directory_async (gio_folder, job.cancellable);
		return folder;
	}
}
