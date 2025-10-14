public class Gth.UnknownViewer : Object, Gth.FileViewer {
	public Gth.ShortcutContext shortcut_context { get { return ShortcutContext.VIEWER; } }

	public void activate (Gth.Window _window) {
		window = _window;
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/unknown-viewer.ui");
		unowned var main_view = builder.get_object ("main_view") as Gtk.Widget;
		scroll_action = (ScrollAction) settings.get_enum (PREF_IMAGE_SCROLL_ACTION);
		window.viewer.set_viewer_widget (main_view);
		window.viewer.set_context_menu (builder.get_object ("context_menu") as Menu);
		window.viewer.viewer_container.add_css_class ("image-view");

		var scroll_events = new Gtk.EventControllerScroll (Gtk.EventControllerScrollFlags.VERTICAL);
		scroll_events.scroll.connect ((controller, dx, dy) => {
			return on_scroll (dx, dy, controller.get_current_event_state ());
		});
		main_view.add_controller (scroll_events);
	}

	public void deactivate () {
		window.viewer.viewer_container.remove_css_class ("image-view");
	}

	public async bool load (FileData file, Job job) {
		return false;
	}

	public void save_preferences () {}
	public void release_resources () {}

	public bool on_scroll (double dx, double dy, Gdk.ModifierType state) {
		if (scroll_action == ScrollAction.CHANGE_FILE) {
			return window.viewer.on_scroll_change_file (dx, dy);
		}
		return false;
	}

	public bool get_pixel_size (out uint width, out uint height) {
		width = 0;
		height = 0;
		return false;
	}

	public void focus () {
	}

	construct {
		settings = new GLib.Settings (GTHUMB_IMAGES_SCHEMA);
		settings.changed.connect ((key) => {
			switch (key) {
			case PREF_IMAGE_SCROLL_ACTION:
				scroll_action = (ScrollAction) settings.get_enum (PREF_IMAGE_SCROLL_ACTION);
				break;
			}
		});
		window = null;
		builder = null;
	}

	weak Gth.Window window;
	GLib.Settings settings;
	Gtk.Builder builder;
	ScrollAction scroll_action;
}
