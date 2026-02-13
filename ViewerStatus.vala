[GtkTemplate (ui = "/app/gthumb/gthumb/ui/viewer-status.ui")]
public class Gth.ViewerStatus : Gtk.Box {
	public void set_n_jobs (uint n) {
		job_status.set_count (n);
		update_visibility ();
	}

	public void set_file_position (int position) {
		if (position < 0) {
			file_position.label = "?";
		}
		else {
			file_position.label = "%d".printf (position + 1);
		}
	}

	public void set_list_info (uint files) {
		total_files.label = " / %u".printf (files);
		navigation_info.visible = (files > 0);
		navigation_buttons.visible = (files > 0);
		update_visibility ();
	}

	void update_visibility () {
		visible = job_status.visible || navigation_info.visible;
	}

	[GtkChild] unowned Gtk.Label total_files;
	[GtkChild] unowned Gtk.Label file_position;
	[GtkChild] unowned Gtk.Box navigation_info;
	[GtkChild] unowned Gtk.Box navigation_buttons;
	[GtkChild] unowned Gth.JobStatus job_status;
}
