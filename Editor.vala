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
			current_editor = tool;
			current_editor.set_window (window);
			current_editor.activate ();
			sidebar_header.title = current_editor.title;
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
		window.editor.content.child = null;
		window.editor.sidebar.child = null;
		window.insert_action_group ("editor", null);
		Util.remove_all_children (left_toolbar);
	}

	public void focus_viewer () {
		if (window.current_page != Window.Page.EDITOR) {
			return;
		}
		if (current_editor != null) {
			// TODO current_editor.focus ();
		}
	}

	public void set_left_toolbar (Gtk.Widget toolbar) {
		left_toolbar.append (toolbar);
	}

	public void hide_apply () {
		apply_button.visible = false;
		header_bar.show_end_title_buttons = true;
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
	ImageTool current_editor;
	Job apply_job = null;
	[GtkChild] public unowned Gtk.Overlay content;
	[GtkChild] public unowned Adw.WindowTitle sidebar_header;
	[GtkChild] public unowned Gth.SidebarResizer sidebar_resizer;
	[GtkChild] public unowned Adw.OverlaySplitView main_view;
	[GtkChild] public unowned Gtk.Button apply_button;
	[GtkChild] public unowned Gtk.MenuButton app_menu_button;
	[GtkChild] public unowned Gtk.Overlay sidebar;
	[GtkChild] unowned Gtk.Box left_toolbar;
	[GtkChild] unowned Adw.HeaderBar header_bar;
}
