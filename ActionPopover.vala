public class Gth.ActionPopover : Gtk.Popover {
	public Gth.ActionList actions;

	public int max_content_height {
		get { return scrolled_window.max_content_height; }
		set { scrolled_window.max_content_height = value; }
	}

	construct {
		add_css_class ("menu");
		add_css_class ("deeper");

		scrolled_window = new Gtk.ScrolledWindow ();
		scrolled_window.hscrollbar_policy = Gtk.PolicyType.NEVER;
		scrolled_window.vscrollbar_policy = Gtk.PolicyType.AUTOMATIC;
		scrolled_window.max_content_height = DEFAULT_MAX_CONTENT_HEIGHT;
		scrolled_window.propagate_natural_height = true;
		child = scrolled_window;

		actions = new Gth.ActionList ();
		scrolled_window.child = actions;
	}

	const int DEFAULT_MAX_CONTENT_HEIGHT = 800;

	Gtk.ScrolledWindow scrolled_window;
}
