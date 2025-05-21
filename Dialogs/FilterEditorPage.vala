[GtkTemplate (ui = "/app/gthumb/gthumb/ui/filter-editor-page.ui")]
public class Gth.FilterEditorPage : Adw.NavigationPage {
	public Gth.Filter filter;

	public signal void cancelled ();
	public signal void save ();

	public void set_filter (Gth.Filter? current_filter) {
		if (current_filter != null) {
			filter = current_filter.duplicate () as Gth.Filter;
		}
		else {
			filter = new Gth.Filter ();
		}
	}

	[GtkCallback]
	private void on_cancel (Gtk.Button source) {
		cancelled ();
	}

	[GtkCallback]
	private void on_save (Gtk.Button source) {
		save ();
	}
}
