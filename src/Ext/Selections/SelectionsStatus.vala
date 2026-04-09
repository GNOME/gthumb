[GtkTemplate (ui = "/org/gnome/gthumb/ui/selections-status.ui")]
public class Gth.SelectionsStatus : Gtk.Box {
	public void set_selection_size (uint number, uint files) {
		var button = (number == 1) ? selection1 : (number == 2) ? selection2 : selection3;
		button.visible = files > 0;
	}

	[GtkChild] unowned Gtk.Button selection1;
	[GtkChild] unowned Gtk.Button selection2;
	[GtkChild] unowned Gtk.Button selection3;
}
