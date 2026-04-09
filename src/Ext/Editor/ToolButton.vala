[GtkTemplate (ui = "/org/gnome/gthumb/ui/tool-button.ui")]
public class Gth.ToolButton : Gtk.Button {
	public new string icon_name {
		set {
			icon.icon_name = value;
		}
	}

	public new string label {
		set {
			text.label = value;
		}
	}

	[GtkChild] unowned Gtk.Label text;
	[GtkChild] unowned Gtk.Image icon;
}
