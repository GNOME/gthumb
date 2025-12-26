[GtkTemplate (ui = "/app/gthumb/gthumb/ui/editor.ui")]
public class Gth.Editor : Gtk.Box {
	public weak Window window {
		get { return _window; }
		set {
			_window = value;
			init ();
		}
	}

	public void activate_tool (ImageTool tool) {
		window.set_page.begin (Window.Page.EDITOR, (_obj, res) => {
			window.set_page.end (res);
			window.insert_action_group ("editor", action_group);
			show_apply ();
			current_editor = tool;
			current_editor.activate (window);
			current_editor.focus ();
			sidebar_main_page.title = current_editor.title;
		});
	}

	public void before_close_page () {
		if (apply_job != null) {
			apply_job.cancel ();
		}
		if (current_editor != null) {
			current_editor.deactivate ();
			current_editor = null;
		}
		content.child = null;
		sidebar_content.child = null;
		window.insert_action_group ("editor", null);
		Util.remove_all_children (left_toolbar);
		set_action_bar (null);
	}

	public void set_left_toolbar (Gtk.Widget widget) {
		left_toolbar.append (widget);
	}

	public void set_content (Gtk.Widget? widget) {
		content.child = widget;
	}

	public void set_options (Gtk.Widget? widget) {
		sidebar_content.child = widget;
	}

	public void set_action_bar (Gtk.Widget? widget) {
		Util.remove_all_children (action_bar);
		if (widget != null) {
			action_bar.append (widget);
			action_bar.visible = true;
		}
		else {
			action_bar.visible = false;
		}
	}

	public void show_apply () {
		apply_button.visible = true;
		sidebar_header.show_end_title_buttons = false;
	}

	public void hide_apply () {
		apply_button.visible = false;
		sidebar_header.show_end_title_buttons = true;
	}

	void init () {
		var builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/editor-menu.ui");
		app_menu_button.menu_model = builder.get_object ("app_menu") as MenuModel;

		sidebar_resizer.add_handle (main_view, Gtk.PackType.START);
		sidebar_resizer.started.connect ((obj) => {
			window.active_resizer = obj;
		});
		sidebar_resizer.ended.connect (() => {
			window.active_resizer = null;
		});

		overview_button.notify["active"].connect (() => {
			if (current_editor != null) {
				overview.main_view = current_editor.image_view;
			}
		});

		current_editor = null;
		init_actions ();
	}

	void init_actions () {
		action_group = new SimpleActionGroup ();

		var action = new SimpleAction ("apply", null);
		action.activate.connect (() => {
			current_editor.apply_changes.begin ((_obj, res) => {
				var edited = current_editor.apply_changes.end (res);
				if (edited) {
					window.set_page.begin (Window.Page.VIEWER);
				}
			});
		});
		action_group.add_action (action);
	}

	weak Window _window;
	public SimpleActionGroup action_group;
	public ImageTool current_editor;
	Job apply_job = null;
	[GtkChild] public unowned Gtk.Overlay content;
	[GtkChild] public unowned Adw.NavigationView sidebar;
	[GtkChild] public unowned Adw.NavigationPage sidebar_main_page;
	[GtkChild] public unowned Gth.SidebarResizer sidebar_resizer;
	[GtkChild] public unowned Adw.OverlaySplitView main_view;
	[GtkChild] public unowned Gtk.Button apply_button;
	[GtkChild] public unowned Gtk.MenuButton app_menu_button;
	[GtkChild] public unowned Gtk.ScrolledWindow sidebar_content;
	[GtkChild] unowned Gtk.Box left_toolbar;
	[GtkChild] unowned Adw.HeaderBar sidebar_header;
	[GtkChild] unowned Gtk.Box action_bar;
	[GtkChild] unowned Gth.ImageOverview overview;
	[GtkChild] unowned Gtk.MenuButton overview_button;
}
