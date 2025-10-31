[GtkTemplate (ui = "/app/gthumb/gthumb/ui/status.ui")]
public class Gth.Status : Gtk.Box {
	public void set_n_jobs (uint n) {
		job_status.set_count (n);
	}

	public void set_list_info (uint files, uint64 size) {
		total_files.label = "%u".printf (files);
		total_size.label = GLib.format_size (size, FormatSizeFlags.DEFAULT);
		list_info.visible = files > 0;
	}

	public void set_selection_info (uint files, uint64 size) {
		selected_files.label = "%u".printf (files);
		selected_size.label = GLib.format_size (size, FormatSizeFlags.DEFAULT);
		selection_info.visible = files > 0;
	}

	public void set_free_space_info (uint64 size) {
		free_space.label = GLib.format_size (size, FormatSizeFlags.DEFAULT);
		free_space_info.visible = size > 0;
	}

	[GtkChild] unowned Gtk.Label total_files;
	[GtkChild] unowned Gtk.Label total_size;
	[GtkChild] unowned Gtk.Box list_info;
	[GtkChild] unowned Gtk.Label selected_files;
	[GtkChild] unowned Gtk.Label selected_size;
	[GtkChild] unowned Gtk.Box selection_info;
	[GtkChild] unowned Gth.JobStatus job_status;
	[GtkChild] unowned Gtk.Box free_space_info;
	[GtkChild] unowned Gtk.Label free_space;
}
