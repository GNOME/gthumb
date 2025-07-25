public class Gth.ImageViewer : Object, Gth.FileViewer {
	public void activate (Gth.Window _window) {
		assert (window == null);
		window = _window;
		image_view = new Gth.ImageView ();
		window.viewer.set_viewer_widget (image_view);
		window.viewer.viewer_container.add_css_class ("image-view");
		init_actions ();
		//builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/image-viewer.ui");

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((event, x, y) => {
			window.viewer.on_motion (x, y);
		});
		image_view.add_controller (motion_events);
	}

	public async void load (FileData file_data) throws Error {
		if (load_job != null) {
			load_job.cancel ();
		}
		var local_job = app.new_job ("Load image %s".printf (file_data.file.get_uri ()));
		load_job = local_job;
		try {
			var image = yield app.image_loader.load_file (file_data.file, local_job.cancellable);
			image_view.set_image (image);
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

	public void deactivate () {
		window.viewer.viewer_container.remove_css_class ("image-view");
		//window.insert_action_group ("image", null);
	}

	public void show () {
	}

	public void hide () {
	}

	void init_actions () {
		var action_group = new SimpleActionGroup ();
		window.insert_action_group ("image", action_group);

		//var action = new SimpleAction ("zoom-fit", null);
		//action.activate.connect ((_action, param) => {
		//});
		//action_group.add_action (action);
	}

	construct {
		//settings = new GLib.Settings (GTHUMB_IMAGE_SCHEMA);
		window = null;
		//builder = null;
		load_job = null;
	}

	weak Gth.Window window;
	//GLib.Settings settings;
	//Gtk.Builder builder;
	Gth.Job load_job;
	Gth.ImageView image_view;
}
