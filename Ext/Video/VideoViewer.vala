public class Gth.VideoViewer : Object, Gth.FileViewer {
	public bool zoom_to_fit {
		get { return _zoom_to_fit; }
		set {
			_zoom_to_fit = value;
			if (picture != null) {
				var align = _zoom_to_fit ? Gtk.Align.FILL : Gtk.Align.CENTER;
				picture.halign = align;
				picture.valign = align;
			}
		}
	}

	public bool playing {
		get { return _playing; }
		set {
			_playing = value;
			if (playbin != null) {
				if (_playing) {
					if (!paused) {
						playbin.set_state (Gst.State.PAUSED);
						skip_to (0);
					}
					else {
						skip_to (get_current_time ());
					}
					playbin.set_state (Gst.State.PLAYING);
				}
				else {
					playbin.set_state (Gst.State.PAUSED);
				}
				wait_playbin_state_change_to_complete ();
			}
		}
	}

	public void activate (Gth.Window _window) {
		assert (window == null);
		assert (picture == null);

		window = _window;
		picture = new Gtk.Picture ();
		picture.halign = Gtk.Align.CENTER;
		picture.valign = Gtk.Align.CENTER;
		zoom_to_fit = settings.get_boolean (PREF_VIDEO_ZOOM_TO_FIT);
		window.viewer.set_viewer_widget (picture);
		window.viewer.viewer_container.add_css_class ("video-view");
		init_actions ();

		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/video-viewer.ui");
		mediabar_revealer = builder.get_object ("mediabar_revealer") as Gtk.Revealer;
		window.viewer.add_viewer_overlay (mediabar_revealer);
		window.viewer.set_left_toolbar (builder.get_object ("left_toolbar") as Gtk.Widget);
		update_rate_label ();

		var click_events = new Gtk.GestureClick ();
		click_events.button = Gdk.BUTTON_PRIMARY;
		click_events.pressed.connect ((n_press, x, y) => {
			playing = !playing;
		});
		picture.add_controller (click_events);

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((event, x, y) => {
			window.viewer.on_motion (x, y);
		});
		picture.add_controller (motion_events);

		unowned var volume_button = builder.get_object ("volume_button") as Gtk.ScaleButton;
		volume_button.notify["active"].connect (() => {
			window.viewer.on_actived_popup (volume_button.active);
		});

		unowned var volume_popover = volume_button.get_popup () as Gtk.Popover;
		if (volume_popover != null) {
			volume_popover.set_position (Gtk.PositionType.TOP);
		}

		unowned var menu_button = builder.get_object ("position_button") as Gtk.MenuButton;
		menu_button.notify["active"].connect ((obj, spec) => {
			window.viewer.on_actived_popup ((obj as Gtk.MenuButton).active);
		});

		menu_button = builder.get_object ("speed_button") as Gtk.MenuButton;
		menu_button.notify["active"].connect ((obj, spec) => {
			window.viewer.on_actived_popup ((obj as Gtk.MenuButton).active);
		});

		menu_button = builder.get_object ("options_button") as Gtk.MenuButton;
		menu_button.notify["active"].connect ((obj, spec) => {
			window.viewer.on_actived_popup ((obj as Gtk.MenuButton).active);
		});

		unowned var adj = builder.get_object ("position_adjustment") as Gtk.Adjustment;
		position_changed_id = adj.value_changed.connect ((local_adj) => {
			var current_time = (int64) (local_adj.get_value () / 100.0 * total_time);
			skip_to (current_time);
			var text = Lib.format_duration_for_display (time_as_microseconds (current_time));
			unowned var label = builder.get_object ("label_position") as Gtk.Label;
			label.set_text (text);
		});

		adj = builder.get_object ("volume_adjustment") as Gtk.Adjustment;
		volume_changed_id = adj.value_changed.connect ((local_adj) => {
			if (playbin != null) {
				// cubic in [0,1], linear in [1,2]
				var volume = local_adj.get_value () / 100.0;
				if (volume <= 1.0) {
					volume = (volume * volume * volume);
				}
				playbin.set ("volume", volume);
				if (volume > 0) {
					playbin.set ("mute", false);
				}
			}
		});

		unowned var widget = builder.get_object ("position_scale") as Gtk.Widget;
		motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((x, y) => {
			update_time_popup_position (x);
		});
		motion_events.enter.connect ((x, y) => {
			update_time_popup_position (x);
			unowned var time_popover = builder.get_object ("time_popover") as Gtk.Popover;
			time_popover.popup ();
		});
		motion_events.leave.connect (() => {
			unowned var time_popover = builder.get_object ("time_popover") as Gtk.Popover;
			time_popover.popdown ();
		});
		widget.add_controller (motion_events);

		create_playbin ();
	}

	public async void load (FileData file_data) throws Error {
		if (playbin == null)
			return;
		playbin.set_state (Gst.State.NULL);
		playbin.set ("uri", file_data.file.get_uri ());
		playing = true;
		// TODO: allow to cancel
		// TODO: yield and resume in on_bus_message
	}

	public void deactivate () {
		if (update_volume_id != 0) {
			Source.remove (update_volume_id);
			update_volume_id = 0;
		}
		if (progress_id != 0) {
			Source.remove (progress_id);
			progress_id = 0;
		}
		window.viewer.viewer_container.remove_css_class ("video-view");
		window.insert_action_group ("video", null);
		if (playbin != null) {
			double volume;
			bool mute;
			playbin.get ("volume", out volume, "mute", out mute);
			settings.set_int (PREF_VIDEO_VOLUME, (int) (volume * 100.0));
			settings.set_boolean (PREF_VIDEO_MUTE, mute);
			settings.set_boolean (PREF_VIDEO_ZOOM_TO_FIT, _zoom_to_fit);

			playbin.set_state (Gst.State.NULL);
			wait_playbin_state_change_to_complete ();

			var bus = playbin.get_bus ();
			bus.remove_signal_watch ();

			playbin = null;
		}
	}

	public void show () {}
	public void hide () {}

	public bool can_save () {
		return false;
	}

	//public async void save_async () throws Error;

	void create_playbin () {
		assert (playbin == null);

		playbin = Gst.ElementFactory.make_full ("playbin",
			"volume", (double) settings.get_int (PREF_VIDEO_VOLUME) / 100.0,
			"mute", settings.get_boolean (PREF_VIDEO_MUTE),
			"force-aspect-ratio", true) as Gst.Pipeline;

		// TODO: check if playbin is null

		// Adjust audio when not playing at normal speed.
		var scaletempo = Gst.ElementFactory.make ("scaletempo");
		if (scaletempo != null) {
			playbin.set ("audio-filter", scaletempo);
		}

		// Rotate according to the orientation tag.
		var videoflip = Gst.ElementFactory.make_full ("videoflip",
			"video-direction", Gst.Video.OrientationMethod.AUTO);
		if (videoflip != null) {
			playbin.set ("video-filter", videoflip);
		}

		// Video sink.
		var gtksink = Gst.ElementFactory.make ("gtk4paintablesink");
		if (gtksink != null) {
			var glsinkbin = Gst.ElementFactory.make_full ("glsinkbin",
				"enable-last-sample", true,
				"sink", gtksink);
			if (glsinkbin != null) {
				playbin.set ("video-sink", glsinkbin);

				Gdk.Paintable sink_paintable;
				gtksink.get ("paintable", out sink_paintable);
				picture.set_paintable (sink_paintable);
			}
		}

		playbin.notify["volume"].connect (() => {
			queue_update_volume ();
		});
		playbin.notify["mute"].connect (() => {
			queue_update_volume ();
		});

		var bus = playbin.get_bus ();
		bus.add_signal_watch ();
		bus.message.connect (on_bus_message);
	}

	void wait_playbin_state_change_to_complete () {
		if (playbin == null)
			return;
		playbin.get_state (null, null, Gst.SECOND * 10);
	}

	uint update_volume_id = 0;

	void queue_update_volume () {
		if (update_volume_id != 0)
			return;
		update_volume_id = Util.next_tick (() => {
			update_volume_id = 0;
			update_volume_from_playbin ();
		});
	}

	void update_volume_from_playbin () {
		if (playbin == null)
			return;
		double volume;
		bool mute;
		playbin.get ("volume", out volume, "mute", out mute);
		if (mute)
			volume = 0.0;

		// cubic in [0,1], linear in [1,2]
		if (volume <= 1.0) {
			volume = Math.exp (1.0 / 3.0 * Math.log (volume)); // cube root of volume
		}

		// Update the volume adjustment.
		unowned var adj = builder.get_object ("volume_adjustment") as Gtk.Adjustment;
		SignalHandler.block (adj, volume_changed_id);
		adj.set_value (volume * 100.0);
		SignalHandler.unblock (adj, volume_changed_id);
	}

	uint progress_id = 0;

	void update_play_button (Gst.State old_state, Gst.State new_state) {
		unowned var play_button = builder.get_object ("play_button") as Gtk.Button;
		unowned var play_icon = builder.get_object ("play_button_image") as Gtk.Image;
		if ((old_state != Gst.State.PLAYING) && (new_state == Gst.State.PLAYING)) {
			play_icon.set_from_icon_name ("pause-large-symbolic");
			play_button.set_tooltip_text (_("Pause"));
			if (progress_id == 0) {
				progress_id = Timeout.add_seconds (1, () => {
					update_current_position_bar ();
					return Source.CONTINUE;
				});
			}
		}
		else if ((old_state == Gst.State.PLAYING) && (new_state != Gst.State.PLAYING)) {
			if (progress_id != 0) {
				Source.remove (progress_id);
				progress_id = 0;
			}
			play_icon.set_from_icon_name ("play-large-symbolic");
			play_button.set_tooltip_text (_("Play"));
		}
	}

	void update_current_position_bar () {
		if (playbin == null) {
			return;
		}
		int64 current_time = 0;
		if (!playbin.query_position (Gst.Format.TIME, out current_time)) {
			return;
		}
		var max_chars = -1;
		if (total_time == 0) {
			playbin.query_duration (Gst.Format.TIME, out total_time);
			int hours, minutes;
			var text = Lib.format_duration_for_display (time_as_microseconds (total_time), out hours, out minutes);
			unowned var label = builder.get_object ("label_duration") as Gtk.Label;
			label.set_text (text);
			max_chars = (hours > 0 ? Util.get_digits (hours) + 1 /* ':' */ : 0)
				+ Util.get_digits (minutes) + 3; // ':00'
			label.width_chars = max_chars;
		}
		current_time = current_time.clamp (0, total_time);
		unowned var adj = builder.get_object ("position_adjustment") as Gtk.Adjustment;
		SignalHandler.block (adj, position_changed_id);
		adj.set_value ((total_time > 0) ? ((double) current_time / total_time) * 100.0 : 0.0);
		SignalHandler.unblock (adj, position_changed_id);

		var text = Lib.format_duration_for_display (time_as_microseconds (current_time));
		unowned var label = builder.get_object ("label_position") as Gtk.Label;
		label.set_text (text);
		if (max_chars > 0) {
			label.width_chars = max_chars;
			unowned var time_popover_label = builder.get_object ("time_popover_label") as Gtk.Label;
			time_popover_label.width_chars = max_chars;
		}
	}

	void reset_state () {
		update_play_button (Gst.State.PLAYING, Gst.State.NULL);
		_playing = false;
		rate = 1.0;
	}

	void on_bus_message (Gst.Message msg) {
		if (msg.src != playbin) {
			return;
		}

		switch (msg.type) {
		case Gst.MessageType.ASYNC_DONE:
			update_current_position_bar ();
			break;

		case Gst.MessageType.STATE_CHANGED:
			var old_state = Gst.State.NULL;
			var new_state = Gst.State.NULL;
			var pending_state = Gst.State.NULL;
			msg.parse_state_changed (out old_state, out new_state, out pending_state);
			if (old_state == new_state)
				break;
			_playing = (new_state == Gst.State.PLAYING);
			paused = (new_state == Gst.State.PAUSED);
			update_current_position_bar ();
			if ((old_state == Gst.State.NULL)
				&& (new_state == Gst.State.READY)
				&& (pending_state != Gst.State.PAUSED))
			{
				update_stream_info ();
				//gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (self));
				//gth_viewer_page_file_loaded (GTH_VIEWER_PAGE (self), self->priv->file_data, self->priv->updated_info, TRUE);
			}
			if ((old_state == Gst.State.READY) && (new_state == Gst.State.PAUSED)) {
				update_stream_info ();
				//gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (self));
				//gth_viewer_page_file_loaded (GTH_VIEWER_PAGE (self), self->priv->file_data, self->priv->updated_info, TRUE);
			}
			if ((old_state == Gst.State.READY) || (new_state == Gst.State.PAUSED)) {
				update_volume_from_playbin ();
			}
			if ((old_state == Gst.State.PLAYING) || (new_state == Gst.State.PLAYING)) {
				update_play_button (old_state, new_state);
			}
			break;

		case Gst.MessageType.DURATION_CHANGED:
			total_time = 0;
			update_current_position_bar ();
			break;

		case Gst.MessageType.EOS:
			if (loop && _playing) {
				skip_to (0);
			}
			else {
				reset_state ();
			}
			break;

		case Gst.MessageType.BUFFERING:
			int percent = 0;
			msg.parse_buffering (out percent);
			playbin.set_state ((percent == 100) ? Gst.State.PLAYING : Gst.State.PAUSED);
			break;

		case Gst.MessageType.ERROR:
			// TODO: gth_viewer_page_file_loaded (GTH_VIEWER_PAGE (self), self->priv->file_data, NULL, FALSE);
			break;

		default:
			break;
		}
	}

	int64 get_current_time () {
		if (builder == null)
			return 0;
		unowned var adj = builder.get_object ("position_adjustment") as Gtk.Adjustment;
		return (int64) ((adj.get_value () / 100.0) * total_time);
	}

	void skip_to (int64 time) {
		if (playbin == null)
			return;
		playbin.seek (rate,	Gst.Format.TIME,
			Gst.SeekFlags.FLUSH | Gst.SeekFlags.ACCURATE,
			Gst.SeekType.SET, time,
			Gst.SeekType.NONE, 0);
	}

	void skip (int seconds) {
		if (playbin == null)
			return;
		var seek_type = Gst.SeekType.SET;
		var seek_flags = Gst.SeekFlags.FLUSH | Gst.SeekFlags.ACCURATE;
		var position = get_current_time () + (seconds * Gst.SECOND);
		if (position < 0) {
			position = 0;
		}
		if (position > total_time) {
			seek_flags |= Gst.SeekFlags.KEY_UNIT | Gst.SeekFlags.SNAP_BEFORE | Gst.SeekFlags.TRICKMODE;
			seek_type = Gst.SeekType.END;
			position = 0;
		}
		playbin.seek (rate,	Gst.Format.TIME, seek_flags,
			seek_type, position,
			Gst.SeekType.NONE, 0);
	}

	void set_rate (double value) {
		rate = value;
		update_rate_label ();
		skip_to (get_current_time ());
	}

	void update_rate_label () {
		unowned var speed_label = builder.get_object ("speed_label") as Gtk.Label;
		double integral;
		var fractional = Math.modf (rate, out integral);
		double _ignored;
		var second_digit = Math.modf (fractional * 10, out _ignored);
		if (fractional == 0) {
			speed_label.label = "%d×".printf ((int) integral);
		}
		else if (second_digit == 0) {
			speed_label.label = "%.1f×".printf (rate);
		}
		else {
			speed_label.label = "%.2f×".printf (rate);
		}
	}

	int get_current_rate_idx () {
		var min_delta = 0.0;
		var min_idx = -1;
		var idx = 0;
		foreach (unowned var value in Rates) {
			var delta = (value - rate).abs ();
			if ((idx == 0) || (delta < min_delta)) {
				min_idx = idx;
				min_delta = delta;
				if (delta == 0) {
					break;
				}
			}
			idx++;
		}
		return (min_idx >= 0) ? min_idx : NORMAL_RATE_IDX;
	}

	inline int time_as_microseconds (int64 time) {
		return (int) (time / 1000000);
	}

	void update_time_popup_position (double x) {
		// Position
		unowned var position_scale = builder.get_object ("position_scale") as Gtk.Widget;
		var max_x = position_scale.get_width ();
		var point_x = ((int) x).clamp (0, max_x);
		unowned var time_popover = builder.get_object ("time_popover") as Gtk.Popover;
		time_popover.pointing_to = { point_x, 0, 1, 1 };

		// Label
		var percent = (x / max_x).clamp (0.0, 1.0);
		var time = time_as_microseconds ((int64) (percent * total_time));
		unowned var time_popover_label = builder.get_object ("time_popover_label") as Gtk.Label;
		time_popover_label.label = Lib.format_duration_for_display (time);
	}

	void init_actions () {
		var action_group = new SimpleActionGroup ();
		window.insert_action_group ("video", action_group);

		var action = new SimpleAction ("decrease-speed", null);
		action.activate.connect ((_action, param) => {
			var idx = get_current_rate_idx ();
			set_rate ((idx > 0) ? Rates[idx - 1] : Rates[0]);
		});
		action_group.add_action (action);

		action = new SimpleAction ("increase-speed", null);
		action.activate.connect ((_action, param) => {
			var idx = get_current_rate_idx ();
			set_rate ((idx < Rates.length - 1) ? Rates[idx + 1] : Rates[Rates.length - 1]);
		});
		action_group.add_action (action);

		action = new SimpleAction ("normal-speed", null);
		action.activate.connect ((_action, param) => {
			set_rate (1.0);
		});
		action_group.add_action (action);

		action = new SimpleAction ("skip", VariantType.INT32);
		action.activate.connect ((_action, param) => {
			var value = (int) param.get_int32 ();
			skip (value);
		});
		action_group.add_action (action);

		action = new SimpleAction ("toggle-play", null);
		action.activate.connect ((_action, param) => {
			playing = !playing;
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-speed", VariantType.DOUBLE, new Variant.double (rate));
		action.activate.connect ((_action, param) => {
			set_rate (param.get_double ());
			_action.set_state (new Variant.double (rate));
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("loop", null, new Variant.boolean (loop));
		action.activate.connect ((_action, param) => {
			loop = Util.toggle_state (_action);
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("zoom-to-fit", null, new Variant.boolean (zoom_to_fit));
		action.activate.connect ((_action, param) => {
			zoom_to_fit = Util.toggle_state (_action);
		});
		action_group.add_action (action);

		action = new SimpleAction ("copy-position-to-clipboard", null);
		action.activate.connect ((_action, param) => {
			var time = time_as_microseconds (get_current_time ());
			var text = Lib.format_duration_not_localized (time);
			window.copy_text_to_clipboard (text);
		});
		action_group.add_action (action);

		action = new SimpleAction ("save-screenshot", null);
		action.activate.connect ((_action, param) => {
			var local_job = window.new_job ("Save screenshot");
			save_screenshot.begin (local_job.cancellable, (_obj, res) => {
				try {
					save_screenshot.end (res);
				}
				catch (Error error) {
					window.show_error (error);
				}
				finally {
					local_job.done ();
				}
			});
		});
		action_group.add_action (action);
	}

	async void save_screenshot (Cancellable cancellable) throws Error {
		var was_playing = playing;
		if (was_playing) {
			playing = false;
		}

		var screenshot = get_playbin_current_frame (playbin);

		// Basename
		var basename = Util.remove_extension (window.viewer.current_file.info.get_display_name ());
		if (basename == null) {
			throw new IOError.FAILED (_("Invalid filename"));
		}

		// Extension
		var preferences = app.get_saver_preferences (SCREENSHOT_TYPE);
		if (preferences == null) {
			throw new IOError.FAILED (_("Invalid filename"));
		}
		var extension = preferences.get_default_extension ();

		// Destination
		var destination = Gth.Settings.get_file (settings, PREF_VIDEO_SCREESHOT_LOCATION);
		if (destination == null) {
			destination = Files.get_special_dir (UserDirectory.PICTURES);
			destination.make_directory_with_parents (cancellable);
			settings.set_string (PREF_VIDEO_SCREESHOT_LOCATION, destination.get_uri ());
		}

		// File
		File file = null;
		for (var attempt = 1; attempt < MAX_ATTEMPTS; attempt++) {
			var display_name = "%s-%02d.%s".printf (basename, attempt, extension);
			var try_file = destination.get_child_for_display_name (display_name);
			if (!try_file.query_exists (cancellable)) {
				file = try_file;
				break;
			}
		}
		if (file == null) {
			throw new IOError.FAILED (_("Invalid filename"));
		}

		yield app.image_saver.save_to_file (file, SCREENSHOT_TYPE, screenshot, cancellable);

		if (was_playing) {
			playing = true;
		}

		// Translators: '%s' is replaced with a filename (do not change it).
		var toast = new Adw.Toast (_("Saved %s").printf (file.get_path ()));
		toast.set_button_label ("Open");
		toast.set_action_name ("app.open-new-window");
		toast.set_action_target_value (new Variant.string (file.get_uri ()));
		window.add_toast (toast);
	}

	void update_stream_info () {
		has_audio = false;
		has_video = false;
		video_width = 0;
		video_height = 0;

		Gst.Element audio_sink = null;
		playbin.get ("audio-sink", out audio_sink);
		if (audio_sink != null) {
			var audio_pad = audio_sink.get_static_pad ("sink");
			if (audio_pad != null) {
				var caps = audio_pad.get_current_caps ();
				if (caps != null) {
					has_audio = true;
				}
			}
		}

		Gst.Element video_sink = null;
		playbin.get ("video-sink", out video_sink);
		if (video_sink != null) {
			var video_pad = video_sink.get_static_pad ("sink");
			if (video_pad != null) {
				var caps = video_pad.get_current_caps ();
				if (caps != null) {
					unowned var structure = caps.get_structure (0);
					if (structure.get_int ("width", out video_width)
						&& structure.get_int ("height", out video_height))
					{
						has_video = true;
					}
				}
			}
		}

		update_zoom_info ();
	}

	void update_zoom_info () {
		/*if (!has_video || (video_width == 0) || (video_height == 0)) {
			window.viewer.status.set_zoom_info (0);
			//window.viewer.status.pixel_info (0, 0);
		}
		else {
			//var view_width = window.viewer.sized_container.width;
			//var view_height = window.viewer.sized_container.height;
			var zoom = (view_width > 0) ? ((double) view_width / video_width) : 0.0;
			window.viewer.status.set_zoom_info (zoom);
			//window.viewer.status.pixel_info (video_width, video_height);
		}*/
	}

	construct {
		settings = new GLib.Settings (GTHUMB_VIDEO_SCHEMA);
		window = null;
		playbin = null;
		picture = null;
		_zoom_to_fit = true;
		_playing = false;
		paused = false;
		rate = 1.0;
		loop = false;
		total_time = 0;
		builder = null;
	}

	GLib.Settings settings;
	weak Gth.Window window;
	Gst.Pipeline playbin;
	Gtk.Picture picture;
	bool _zoom_to_fit;
	bool _playing;
	Gtk.Builder builder;
	int64 total_time;
	ulong position_changed_id;
	ulong volume_changed_id;
	double rate;
	bool loop;
	unowned Gtk.Revealer mediabar_revealer;
	bool paused = false;
	int video_width = 0;
	int video_height = 0;
	bool has_audio = false;
	bool has_video = false;

	const double[] Rates = {
		0.03, 0.05, 0.12, 0.25, 0.33, 0.5, 0.75,
		1.0, // Note: Keep NORMAL_RATE_IDX up-to-date
		1.25, 1.5, 1.75, 2.0, 3.0, 4.0
	};
	const int NORMAL_RATE_IDX = 7;
	const int MAX_ATTEMPTS = 1000;
	const string SCREENSHOT_TYPE = "image/jpeg";
}
