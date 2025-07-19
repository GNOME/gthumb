[GtkTemplate (ui = "/app/gthumb/gthumb/ui/viewer.ui")]
public class Gth.Viewer : Gtk.Box {
	public enum ViewFlags {
		DEFAULT,
		KEEP_CURRENT_PAGE,
		NO_DELAY,
		FULLSCREEN,
	}

	public weak Window window {
		get { return _window; }
		set {
			_window = value;
			init ();
		}
	}

	public Gth.FileData current_file;

	Gth.Job load_job = null;
	FileViewer current_viewer = null;

	public async void load_file (FileData file, ViewFlags flags = ViewFlags.DEFAULT) throws Error {
		if (load_job != null) {
			load_job.cancel ();
		}
		var local_job = window.new_job ("Load %s".printf (file.file.get_uri ()));
		load_job = local_job;
		try {
			if ((current_viewer == null) || !app.viewer_can_view (current_viewer.get_class ().get_type (), file.get_content_type ())) {
				if (current_viewer != null) {
					current_viewer.deactivate ();
				}
				current_viewer = app.get_viewer_for_type (file.get_content_type ());
				if (current_viewer == null) {
					throw new IOError.FAILED (_("Cannot load this kind of file"));
				}
				current_viewer.activate (window);
			}
			yield current_viewer.load (file);
			if (!(ViewFlags.KEEP_CURRENT_PAGE in flags)) {
				window.set_page (Window.Page.VIEWER);
			}
		}
		catch (Error error) {
			stdout.printf ("ERROR: %s\n", error.message);
			local_job.error = error;
		}
		local_job.done ();
		if (load_job == local_job) {
			load_job = null;
		}
		if (local_job.error != null) {
			throw local_job.error;
		}
	}

	public bool is_loading () {
		return (load_job != null) && load_job.is_running ();
	}

	public void view_file (FileData file, ViewFlags flags = ViewFlags.DEFAULT) {
		load_file.begin (file, flags, (_obj, res) => {
			try {
				load_file.end (res);
			}
			catch (Error error) {
				window.show_error (error);
			}
		});
	}

	public void set_viewer_widget (Gtk.Widget? widget) {
		if (viewer_container.child != null) {
			viewer_container.child.destroy ();
		}
		viewer_container.child = widget;
	}

	public void add_viewer_overlay (Gtk.Widget widget) {
		viewer_container.add_overlay (widget);
	}

	public void remove_viewer_overlay (Gtk.Widget widget) {
		viewer_container.remove_overlay (widget);
	}

	public void before_close_page () {
		if (current_viewer != null) {
			current_viewer.deactivate ();
			current_viewer = null;
		}
	}

	public void after_show_page () {
		if (current_viewer != null) {
			current_viewer.show ();
		}
	}

	void init () {
		current_file = null;

		sidebar_resizer.add_handle (viewer_view, Gtk.PackType.START);
		sidebar_resizer.started.connect ((obj) => {
			window.active_resizer = obj;
		});
		sidebar_resizer.ended.connect (() => {
			window.active_resizer = null;
		});

		init_actions ();
	}

	void init_actions () {
		var builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/app-menu.ui");
		app_menu_button.menu_model = builder.get_object ("app_menu") as MenuModel;

		var action_group = window.action_group;

		var action = new SimpleAction.stateful ("viewer-properties", null, new Variant.boolean (viewer_view.show_sidebar));
		action.activate.connect ((_action, param) => {
			viewer_view.show_sidebar = Util.toggle_state (_action);
		});
		action_group.add_action (action);
	}

	[GtkChild] unowned Adw.OverlaySplitView viewer_view;
	[GtkChild] unowned Gth.SidebarResizer sidebar_resizer;
	[GtkChild] unowned Gtk.MenuButton app_menu_button;
	[GtkChild] public unowned Gtk.Overlay viewer_container;
	weak Window _window;
}
