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
			current_editor.activate (window);
		});
	}

	public void before_close_page () {
		if (current_editor != null) {
			current_editor.deactivate ();
			current_editor = null;
		}
		window.editor.content.child = null;
		window.editor.sidebar.child = null;
		window.insert_action_group ("editor", null);
	}

	public void focus_viewer () {
		if (window.current_page != Window.Page.EDITOR) {
			return;
		}
		if (current_editor != null) {
			// TODO current_editor.focus ();
		}
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
		action.activate.connect ((_action, param) => {
			// TODO
		});
		action_group.add_action (action);
	}

	weak Window _window;
	public SimpleActionGroup action_group;
	ImageTool current_editor;
	[GtkChild] public unowned Gtk.Overlay content;
	[GtkChild] public unowned Adw.WindowTitle sidebar_header;
	[GtkChild] public unowned Gth.SidebarResizer sidebar_resizer;
	[GtkChild] public unowned Adw.OverlaySplitView main_view;
	[GtkChild] public unowned Gtk.Button apply_button;
	[GtkChild] public unowned Gtk.MenuButton app_menu_button;
	[GtkChild] public unowned Gtk.Overlay sidebar;
}
