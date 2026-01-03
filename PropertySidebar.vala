[GtkTemplate (ui = "/app/gthumb/gthumb/ui/property-sidebar.ui")]
public class Gth.PropertySidebar : Gtk.Box {
	public Gth.FileData current_file {
		get { return _file; }
		set {
			if (value != _file) {
				_file = value;
			}
			update_view ();
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

	public async void load (Gth.FileData file_data, Cancellable cancellable) throws Error {
		var source = new FileSourceVfs ();
		var result = yield source.read_metadata (file_data.file, "*", cancellable);
		file_data.update_info (result.info);
		current_file = file_data;
	}

	public void set_selection_info (uint files, uint64 size) {
		selection_view.set_selection_info (files, size);
		current_file = null;
	}

	public bool has_file (File file) {
		return (current_file != null) && current_file.file.equal (file);
	}

	public void update_view () {
		string first_visible_page = null;
		var active_page_is_empty = false;
		var always_show_exif = (_file != null) ? Util.content_type_is_image (_file.get_content_type ()) : true;
		var visible_pages = 0;
		foreach (unowned var page in pages) {
			var visible = ((_file == null) && (page.view == selection_view))
				|| ((_file != null) && page.view.can_view (_file));
			if (visible) {
				page.view.set_file (_file);
				page.button.sensitive = true;
				page.button.visible = true;
				 if (first_visible_page == null) {
					first_visible_page = page.name;
				}
				visible_pages++;
			}
			else {
				page.view.set_file (null);
				page.button.sensitive = false;
				if ((page.name == "other-properties") || (page.name == "selection")) {
					page.button.visible = false;
				}
				else {
					page.button.visible = always_show_exif;
				}
				if (page.button.active) {
					active_page_is_empty = true;
				}
			}
		}
		if (active_page_is_empty) {
			if (first_visible_page == null) {
				first_visible_page = "file-properties";
			}
			set_page (first_visible_page);
		}
		bottom_bar.visible = (visible_pages > 1) || search_bar.visible;
	}

	construct {
		orientation = Gtk.Orientation.VERTICAL;
		pages = new GenericArray<PageInfo?>();
		_file = null;

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
		add_page (new Gth.OtherPropertyView ());
		selection_view = new Gth.SelectionView ();
		add_page (selection_view);
		set_page ("file-properties");
	}

	void add_page (Gth.PropertyView view, bool use_icon = false) {
		var button = new Gtk.ToggleButton ();
		if (use_icon) {
			button.child = new Gtk.Image.from_icon_name (view.get_icon ());
			button.tooltip_text = view.get_title ();
		}
		else {
			button.child = new Gtk.Label (view.get_title ());
		}
		button.action_name = "sidebar.set-view";
		button.action_target = new Variant.string (view.get_id ());
		button.add_css_class ("flat");
		header.append (button);

		stack.add_named (view, view.get_id ());

		pages.add ({ view, button, view.get_id () });
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

	[GtkChild] unowned Adw.WrapBox header;
	[GtkChild] unowned Gtk.Stack stack;
	[GtkChild] unowned Gtk.Box search_bar;
	[GtkChild] unowned Gtk.Box bottom_bar;
	[GtkChild] public unowned Gth.SidebarResizer resizer;

	struct PageInfo {
		weak Gth.PropertyView view;
		weak Gtk.ToggleButton button;
		string name;
	}

	GenericArray<PageInfo?> pages;
	Gth.FileData _file;
	SelectionView selection_view;
}
