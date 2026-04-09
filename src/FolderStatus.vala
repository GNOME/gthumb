[GtkTemplate (ui = "/org/gnome/gthumb/ui/folder-status.ui")]
public class Gth.FolderStatus : Gtk.Box {
	public Icon gicon {
		set {
			icon_image.gicon = value;
		}
	}

	public string icon_name {
		set {
			icon_image.icon_name = value;
		}
	}

	public string title {
		set {
			title_label.label = value;
			title_label.visible = (value != "");
		}
	}

	public string description {
		set {
			description_label.label = value;
			description_label.visible = (value != "");
		}
	}

	public bool dimmed_icon {
		set {
			if (value) {
				icon_image.add_css_class ("dimmed");
			}
			else {
				icon_image.remove_css_class ("dimmed");
			}
		}
	}

	public int icon_size {
		set {
			icon_image.pixel_size = value;
		}
	}

	[GtkChild] unowned Gtk.Image icon_image;
	[GtkChild] unowned Gtk.Label title_label;
	[GtkChild] unowned Gtk.Label description_label;
}
