[GtkTemplate (ui = "/app/gthumb/gthumb/ui/editor-sidebar.ui")]
public class Gth.EditorSidebar : Gtk.Box {
	[GtkChild] public unowned Gth.SidebarResizer resizer;

	public string title {
		set {
			header.title = value;
		}
	}

	[GtkChild] unowned Adw.WindowTitle header;
}
