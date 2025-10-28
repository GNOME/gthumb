[GtkTemplate (ui = "/app/gthumb/gthumb/ui/tool-button.ui")]
public class Gth.ToolButton : Gtk.Button {
	public string icon_name {
		set {
			icon.icon_name = value;
		}
	}

	public string label {
		set {
			text.label = value;
		}
	}

	[GtkChild] unowned Gtk.Label text;
	[GtkChild] unowned Gtk.Image icon;
}
