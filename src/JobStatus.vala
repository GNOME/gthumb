[GtkTemplate (ui = "/org/gnome/gthumb/ui/job-status.ui")]
public class Gth.JobStatus : Gtk.Button {
	public void set_count (uint count) {
		job_count.label = "%u".printf (count);
		visible = (count > 0);
	}

	[GtkChild] unowned Gtk.Label job_count;
}
