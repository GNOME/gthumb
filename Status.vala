[GtkTemplate (ui = "/app/gthumb/gthumb/ui/status.ui")]
public class Gth.Status : Gtk.Box {
	public void set_n_jobs (uint n) {
		//jobs.set_text ("Jobs: %u".printf (n));
		//jobs.visible = (n > 0);
		spinner.visible = (n > 0);
	}
	[GtkChild] Gtk.Label jobs;
	[GtkChild] Adw.Spinner spinner;
}
