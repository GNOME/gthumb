[GtkTemplate (ui = "/app/gthumb/gthumb/ui/property-sidebar.ui")]
public class Gth.PropertySidebar : Gtk.Box {
	construct {
		orientation = Gtk.Orientation.VERTICAL;
		pages = new GenericArray<PageInfo?>();

		var action_group = new SimpleActionGroup ();
		insert_action_group ("sidebar", action_group);

		var action = new SimpleAction ("set-view", GLib.VariantType.STRING);
		action.activate.connect ((_action, param) => {
			set_page (param.get_string ());
		});
		action_group.add_action (action);

		add_page (new Gth.FilePropertyView ());
		add_page (new Gth.EmbeddedPropertyView ());
		set_page ("file-properties");
	}

	void add_page (Gth.PropertyView view) {
		var button = new Gtk.ToggleButton ();
		button.child = new Gtk.Image.from_icon_name (view.get_icon ());
		button.action_name = "sidebar.set-view";
		button.action_target = new Variant.string (view.get_name ());
		button.hexpand = true;
		button.add_css_class ("flat");
		button.tooltip_text = view.get_title ();
		header.append (button);

		stack.add_named (view, view.get_name ());

		pages.add ({ view, button, view.get_name () });
	}

	public void set_file (Gth.FileData? file_data) {
		foreach (unowned var page in pages) {
			if ((file_data != null) && page.view.can_view (file_data)) {
				page.view.set_file (file_data);
				page.button.sensitive = true;
			}
			else {
				page.view.set_file (null);
				page.button.sensitive = false;
			}
		}
	}

	public void set_page (string name) {
		stack.set_visible_child_name (name);
		foreach (unowned var page in pages) {
			page.button.active = (page.name == name);
		}
	}

	[GtkChild] unowned Gtk.Box header;
	[GtkChild] unowned Gtk.Stack stack;

	struct PageInfo {
		weak Gth.PropertyView view;
		weak Gtk.ToggleButton button;
		string name;
	}

	GenericArray<PageInfo?> pages;
}
