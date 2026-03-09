[GtkTemplate (ui = "/app/gthumb/gthumb/ui/slideshow-window.ui")]
public class Gth.Slideshow : Gth.Window {
	public GenericList<FileData> files {
		set {
			var iter = value.iterator ();
			_files = iter.filter ((file_data) => {
				return Util.content_type_is_image (file_data.get_content_type ());
			});
		}
	}

	public override void add_toast (Adw.Toast toast) {
		toast_overlay.dismiss_all ();
		toast_overlay.add_toast (toast);
	}

	public override void before_closing () {
		cancel_automatic ();
		preloader.cancel ();
		if (load_job != null) {
			load_job.cancel ();
		}
	}

	public override void init_actions () {
		base.init_actions ();

		var action = new SimpleAction ("next-image", null);
		action.activate.connect (() => next ());
		action_group.add_action (action);

		action = new SimpleAction ("previous-image", null);
		action.activate.connect (() => previous ());
		action_group.add_action (action);

		action = new SimpleAction ("toggle-play", null);
		action.activate.connect (() => toggle_play ());
		action_group.add_action (action);
	}

	async void load_position (uint position) {
		if (load_job != null) {
			load_job.cancel ();
		}
		cancel_automatic ();
		var file_data = _files[(int) position];
		if (file_data == null) {
			restart ();
			return;
		}
		var local_job = new_job (_("Loading %s").printf (file_data.get_display_name ()),
			JobFlags.FOREGROUND,
			"gth-content-loading-symbolic"
		);
		load_job = local_job;
		try {
			preloader.cancel ();
			if (current_file != null) {
				preloader.cache.touch (current_file.file);
			}
			var image = preloader.cache[file_data.file];
			if (image == null) {
				var requested_size = get_requested_size ();
				// stdout.printf ("> LOAD: %s (requested_size: %u)\n",
				// 	file_data.get_display_name (),
				// 	requested_size);
				image = yield app.image_loader.load_file (
					monitor_profile,
					file_data.file,
					LoadFlags.DEFAULT,
					local_job.cancellable,
					requested_size
				);
			}
			current_file = file_data;
			current_position = position;
			image_view.image = image;
			image_view.grab_focus ();
			preload_some_files ();
			if (automatic) {
				next_id = Timeout.add_seconds_once (delay, () => next ());
			}
		}
		catch (Error error) {
			show_error (error);
		}
		finally {
			local_job.done ();
			if (load_job == local_job) {
				load_job = null;
			}
		}
	}

	public void preload_some_files () {
		preloader.cancel ();
		var queue = new Queue<File>();

		void add_to_queue (uint pos) {
			var file_data = _files.model.get_item (pos) as FileData;
			if (file_data == null) {
				return;
			}
			if (file_data.file in preloader.cache) {
				return;
			}
			// stdout.printf ("> ADD TO QUEUE: %s\n", file_data.file.get_uri ());
			queue.push_tail (file_data.file);
		}

		add_to_queue (current_position + 1);
		add_to_queue (current_position + 2);
		if (queue.is_empty ()) {
			return;
		}
		var local_job = new_job ("Preload", JobFlags.HIDDEN);
		var requested_size = get_requested_size ();
		// stdout.printf ("> PRELOAD: requested_size: %u\n", requested_size);
		preloader.load.begin (monitor_profile, queue, local_job, requested_size, (_obj, res) => {
			try {
				preloader.load.end (res);
			}
			catch (Error error) {
			}
			finally {
				// preloader.cache.print ();
				local_job.done ();
			}
		});
	}

	uint get_requested_size () {
		int width, height;
		monitor_profile.get_geometry (out width, out height);
		return (uint) int.max (width, height);
	}

	void toggle_play () {
		if (automatic) {
			show_message (_("Paused"));
			automatic = false;
			cancel_automatic ();
		}
		else {
			automatic = true;
			next ();
		}
	}

	void cancel_automatic () {
		if (next_id != 0) {
			Source.remove (next_id);
			next_id = 0;
		}
	}

	void start () {
		if (random_order) {
			_files.sort ((a, b) => GLib.Random.int_range (-1, 2));
		}
		load_position.begin (0);
	}

	void next () {
		load_position.begin (current_position + 1);
	}

	void previous () {
		if (current_position > 0) {
			load_position.begin (current_position - 1);
		}
	}

	void restart () {
		if (loop) {
			start ();
		}
		else {
			close ();
		}
	}

	bool on_key_pressed (Gtk.EventControllerKey controller, uint keyval, uint keycode, Gdk.ModifierType state) {
		var context = ShortcutContext.SLIDESHOW;
		var shortcut = app.shortcuts.find_by_key (context, keyval, state);
		if ((shortcut == null) || (ShortcutContext.DOC in shortcut.context)) {
			// stdout.printf ("> NULL\n");
			return false;
		}
		// stdout.printf ("> shortcut: '%s'\n", shortcut.detailed_action);
		activate_action_variant (shortcut.action_name, shortcut.action_parameter);
		return true;
	}

	construct {
		settings = new GLib.Settings (GTHUMB_SLIDESHOW_SCHEMA);
		automatic = settings.get_boolean (PREF_SLIDESHOW_AUTOMATIC);
		delay = settings.get_int (PREF_SLIDESHOW_DELAY);
		loop = settings.get_boolean (PREF_SLIDESHOW_LOOP);
		random_order = settings.get_boolean (PREF_SLIDESHOW_RANDOM_ORDER);
		preloader = new Preloader ();
		current_file = null;
		load_job = null;
		current_position = 0;
		next_id = 0;
		map.connect (() => start ());
		var key_events = new Gtk.EventControllerKey ();
		key_events.key_pressed.connect (on_key_pressed);
		image_view.add_controller (key_events);
		image_view.cursor = new Gdk.Cursor.from_name ("none", null);
	}

	[GtkChild] unowned Adw.ToastOverlay toast_overlay;
	[GtkChild] unowned Gth.ImageView image_view;
	GenericList<FileData> _files;
	bool automatic;
	uint delay;
	bool loop;
	bool random_order;
	Preloader preloader;
	FileData current_file;
	Job load_job;
	uint current_position;
	uint next_id;
	GLib.Settings settings;
}
