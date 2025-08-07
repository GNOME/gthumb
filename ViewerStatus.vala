[GtkTemplate (ui = "/app/gthumb/gthumb/ui/viewer-status.ui")]
public class Gth.ViewerStatus : Gtk.Box {
	public void set_n_jobs (uint n) {
		spinner.visible = (n > 0);
	}

	public void set_file_position (uint position) {
		file_position.label = "%u".printf (position + 1);
	}

	public void set_list_info (uint files) {
		total_files.label = " / %u".printf (files);
	}

	public void set_pixel_info (uint width, uint height) {
		pixel_info.label = "%u×%u".printf (width, height);
		pixel_info.visible = ((width > 0) && (height > 0));
	}

	public void set_zoom_info (double zoom) {
		zoom_info.label = "%d%%".printf ((int) (zoom * 100.0));
		zoom_info.visible = (zoom > 0);
	}

	[GtkChild] unowned Gtk.Label total_files;
	[GtkChild] unowned Gtk.Label file_position;
	[GtkChild] unowned Gtk.Label zoom_info;
	[GtkChild] unowned Gtk.Label pixel_info;
	[GtkChild] unowned Adw.Spinner spinner;
}
