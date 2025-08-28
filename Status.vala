[GtkTemplate (ui = "/app/gthumb/gthumb/ui/status.ui")]
public class Gth.Status : Gtk.Box {
	public void set_n_jobs (uint n) {
		job_status.set_count (n);
	}

	public void set_list_info (uint files, uint64 size) {
		total_files.label = "%u".printf (files);
		total_size.label = GLib.format_size (size, FormatSizeFlags.DEFAULT);
		total_size.visible = files > 0;
	}

	[GtkChild] unowned Gtk.Label total_files;
	[GtkChild] unowned Gtk.Label total_size;
	[GtkChild] unowned Gth.JobStatus job_status;
}
