[GtkTemplate (ui = "/app/gthumb/gthumb/ui/property-sidebar.ui")]
public class Gth.PropertySidebar : Gtk.Box {
	public Gth.FileData file_data {
		get { return _file_data; }
		set {
			_file_data = value;
			foreach (unowned var page in pages) {
				if ((_file_data != null) && page.view.can_view (_file_data)) {
					page.view.set_file (_file_data);
					page.button.sensitive = true;
				}
				else {
					page.view.set_file (null);
					page.button.sensitive = false;
				}
			}
		}
	}

	public void set_page (string name) {
		stack.set_visible_child_name (name);
		foreach (unowned var page in pages) {
			var active = (page.name == name);
			page.button.active = active;
			if (active) {
				search_bar.visible = page.view.with_search ();
			}
		}
	}

	construct {
		orientation = Gtk.Orientation.VERTICAL;
		pages = new GenericArray<PageInfo?>();
		_file_data = null;

		var action_group = new SimpleActionGroup ();
		insert_action_group ("sidebar", action_group);

		var action = new SimpleAction ("set-view", GLib.VariantType.STRING);
		action.activate.connect ((_action, param) => {
			set_page (param.get_string ());
		});
		action_group.add_action (action);

		add_page (new Gth.FilePropertyView ());
		add_page (new Gth.ExifPropertyView ());
		add_page (new Gth.IptcPropertyView ());
		add_page (new Gth.XmpPropertyView ());
		//add_page (new Gth.MetadataView ());
		set_page ("file-properties");
	}

	void add_page (Gth.PropertyView view) {
		var button = new Gtk.ToggleButton ();
		button.child = new Gtk.Label (view.get_title ());
		button.action_name = "sidebar.set-view";
		button.action_target = new Variant.string (view.get_name ());
		button.hexpand = true;
		button.add_css_class ("flat");
		header.append (button);

		stack.add_named (view, view.get_name ());

		pages.add ({ view, button, view.get_name () });
	}

	[GtkCallback]
	void on_search_changed (Gtk.SearchEntry entry) {
		var text = entry.text.casefold ();
		foreach (unowned var page in pages) {
			if (page.view.with_search ()) {
				page.view.set_search (text);
			}
		}
	}

	[GtkChild] unowned Gtk.Box header;
	[GtkChild] unowned Gtk.Stack stack;
	[GtkChild] unowned Gtk.Box search_bar;

	struct PageInfo {
		weak Gth.PropertyView view;
		weak Gtk.ToggleButton button;
		string name;
	}

	GenericArray<PageInfo?> pages;
	Gth.FileData _file_data;
}
