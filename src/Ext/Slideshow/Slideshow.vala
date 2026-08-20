[GtkTemplate (ui = "/org/gnome/gthumb/ui/slideshow-window.ui")]
public class Gth.Slideshow : Gth.Window {
	public GenericList<FileData> files;

	public override void add_toast (Adw.Toast toast) {
		toast_overlay.dismiss_all ();
		toast_overlay.add_toast (toast);
	}

	public override void before_closing () {
		cancel_next ();
		cancel_hide_cursor ();
		preloader.cancel ();
		if (load_job != null) {
			load_job.cancel ();
		}
	}

	public override void init_actions () {
		base.init_actions ();

		var action = new SimpleAction ("next-image", null);
		action.activate.connect (() => {
			if (!next ()) {
				edge_reached ();
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("previous-image", null);
		action.activate.connect (() => {
			if (!previous ()) {
				edge_reached ();
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("toggle-play", null);
		action.activate.connect (() => toggle_play ());
		action_group.add_action (action);
	}

	async void load_position (uint position) {
		if (load_job != null) {
			load_job.cancel ();
		}
		cancel_next ();
		var file_data = files[(int) position];
		if (file_data == null) {
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
			preload_some_files ();
			if (automatic) {
				queue_next ();
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
			var file_data = files.model.get_item (pos) as FileData;
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
		add_to_queue (current_position - 1);
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

	void queue_next () {
		next_id = Timeout.add_seconds_once (delay, () => {
			if (!next ()) {
				restart ();
			}
		});
	}

	void pause () {
		automatic = false;
		if (load_job != null) {
			load_job.cancel ();
		}
		cancel_next ();
	}

	void toggle_play () {
		if (automatic) {
			show_message (_("Paused"));
			pause ();
		}
		else {
			toast_overlay.dismiss_all ();
			automatic = true;
			if (!next ()) {
				restart ();
			}
		}
	}

	void cancel_next () {
		if (next_id != 0) {
			Source.remove (next_id);
			next_id = 0;
		}
	}

	void start () {
		if (random_order) {
			files.sort ((a, b) => GLib.Random.int_range (-1, 2));
		}
		if (files.length () == 1) {
			automatic = false;
		}
		load_position.begin (0);
	}

	bool can_load_next () {
		return current_position < files.length () - 1;
	}

	bool next () {
		if (!can_load_next ()) {
			return false;
		}
		load_position.begin (current_position + 1);
		return true;
	}

	bool can_load_previous () {
		return current_position > 0;
	}

	bool previous () {
		if (!can_load_previous ()) {
			return false;
		}
		load_position.begin (current_position - 1);
		return true;
	}

	void restart () {
		if (loop) {
			if (files.length () > 1) {
				start ();
			}
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

	void hide_cursor_after_timeout () {
		cancel_hide_cursor ();
		hide_cursor_id = Timeout.add_seconds (1, () => {
			hide_cursor_id = 0;
			set_cursor_visible (false);
			return Source.REMOVE;
		});
	}

	void cancel_hide_cursor () {
		if (hide_cursor_id != 0) {
			Source.remove (hide_cursor_id);
			hide_cursor_id = 0;
		}
	}

	void set_cursor_visible (bool visible) {
		swipeable_view.cursor = visible ? null : new Gdk.Cursor.from_name ("none", null);
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
		fullscreened = true;

		swipeable_view.change_content.connect ((direction) => {
			if (direction == NavigationDirection.FORWARD) {
				next ();
			}
			else {
				previous ();
			}
		});

		swipeable_view.can_change_content.connect_after ((direction, out can_change) => {
			if (direction == NavigationDirection.FORWARD) {
				can_change = can_load_next ();
			}
			else {
				can_change = can_load_previous ();
			}
		});

		swipeable_view.scroll_begin.connect (() => pause ());

		swipeable_view.scroll_end.connect (() => {
			if (!automatic) {
				automatic = true;
				if (load_job == null) {
					queue_next ();
				}
			}
		});

		// image_view.cursor = new Gdk.Cursor.from_name ("none", null);

		var key_events = new Gtk.EventControllerKey ();
		key_events.key_pressed.connect (on_key_pressed);
		swipeable_view.add_controller (key_events);

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect (() => {
			set_cursor_visible (true);
			hide_cursor_after_timeout ();
		});
		swipeable_view.add_controller (motion_events);

		var scroll_events = new Gtk.EventControllerScroll (Gtk.EventControllerScrollFlags.VERTICAL);
		scroll_events.scroll.connect ((controller, dx, dy) => {
			if (dy > 0) {
				if (!next ()) {
					edge_reached ();
				}
			}
			else {
				if (!previous ()) {
					edge_reached ();
				}
			}
		});
		swipeable_view.add_controller (scroll_events);

		var seconday_click_events = new Gtk.GestureClick ();
		seconday_click_events.set_button (Gdk.BUTTON_SECONDARY);
		seconday_click_events.pressed.connect ((n_press, x, y) => close ());
		swipeable_view.add_controller (seconday_click_events);
	}

	[GtkChild] unowned Adw.ToastOverlay toast_overlay;
	[GtkChild] unowned Gth.SwipeableView swipeable_view;
	[GtkChild] unowned Gth.ImageView image_view;
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
	uint hide_cursor_id = 0;
}
