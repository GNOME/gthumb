using GLib;

namespace Gth.Util {
	[CCode(cname = "gtk_drag_icon_get_for_drag", cprefix = "", cheader_filename = "")]
	public static unowned Gtk.DragIcon get_drag_icon_for_drag (Gdk.Drag drag);
}
