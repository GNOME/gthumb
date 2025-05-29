[GtkTemplate (ui = "/app/gthumb/gthumb/ui/status.ui")]
public class Gth.Status : Gtk.Box {
	public void set_n_jobs (uint n) {
		//jobs.set_text ("Jobs: %u".printf (n));
		//jobs.visible = (n > 0);
		spinner.visible = (n > 0);
	}

	public void set_list_info (uint files, uint64 size) {
		total_files.label = "%u".printf (files);
		total_size.label = GLib.format_size (size, FormatSizeFlags.DEFAULT);
	}

	public void set_selection_info (uint files, uint64 size) {
		selected_files.label = "%u".printf (files);
		selected_size.label = GLib.format_size (size, FormatSizeFlags.DEFAULT);
		selection_info.visible = files > 0;
	}

	[GtkChild] Gtk.Label jobs;
	[GtkChild] Gtk.Label total_files;
	[GtkChild] Gtk.Label total_size;
	[GtkChild] Gtk.Label selected_files;
	[GtkChild] Gtk.Label selected_size;
	[GtkChild] Gtk.Box selection_info;
	[GtkChild] Adw.Spinner spinner;
}
