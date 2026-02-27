public class Gth.VideoViewer : Object, Gth.FileViewer {
	public Gth.ShortcutContext shortcut_context { get { return ShortcutContext.MEDIA_VIEWER; } }

	public bool zoom_to_fit {
		get { return _zoom_to_fit; }
		set {
			_zoom_to_fit = value;
			if (video_view != null) {
				video_view.zoom_type = _zoom_to_fit ? ZoomType.MAXIMIZE : ZoomType.MAXIMIZE_IF_LARGER;
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
		assert (video_view == null);

		window = _window;

		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/video-viewer.ui");
		view_stack = builder.get_object ("view_stack") as Gtk.Stack;
		video_view = builder.get_object ("video_view") as Gth.VideoView;
		audio_view = builder.get_object ("audio_view") as Adw.StatusPage;
		empty_view = builder.get_object ("empty_view") as Gtk.Widget;
		audio_status = builder.get_object ("audio_status") as Gth.FolderStatus;

		zoom_to_fit = settings.get_boolean (PREF_VIDEO_ZOOM_TO_FIT);
		scroll_action = (ScrollAction) settings.get_enum (PREF_VIDEO_SCROLL_ACTION);
		window.viewer.set_viewer_widget (view_stack);
		window.viewer.viewer_container.add_css_class ("video-view");
		window.viewer.set_left_toolbar (builder.get_object ("left_toolbar") as Gtk.Widget);

		mediabar = builder.get_object ("mediabar") as Gtk.Widget;
		window.viewer.set_mediabar (mediabar, Gtk.Align.FILL);

		var click_events = new Gtk.GestureClick ();
		click_events.button = Gdk.BUTTON_PRIMARY;
		click_events.pressed.connect ((n_press, x, y) => {
			playing = !playing;
			focus ();
		});
		video_view.add_controller (click_events);

		click_events = new Gtk.GestureClick ();
		click_events.button = Gdk.BUTTON_PRIMARY;
		click_events.pressed.connect ((n_press, x, y) => {
			playing = !playing;
			focus ();
		});
		audio_view.add_controller (click_events);

		unowned var menu_button = builder.get_object ("position_button") as Gtk.MenuButton;
		menu_button.notify["active"].connect ((obj, spec) => {
			window.viewer.on_actived_popup (((Gtk.MenuButton) obj).active);
		});

		menu_button = builder.get_object ("options_button") as Gtk.MenuButton;
		menu_button.notify["active"].connect ((obj, spec) => {
			window.viewer.on_actived_popup (((Gtk.MenuButton) obj).active);
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
		var motion_events = new Gtk.EventControllerMotion ();
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
		init_actions ();
	}

	public async bool load (FileData file_data, Job job) throws Error {
		if (playbin == null) {
			return false;
		}
		reset_state ();
		reset_view ();
		playbin.set_state (Gst.State.NULL);
		playbin.set ("uri", file_data.file.get_uri ());
		playing = true;
		// TODO: allow to cancel
		// TODO: yield and resume in on_bus_message
		return true;
	}

	public void deactivate () {
		window.viewer.viewer_container.remove_css_class ("video-view");
		window.insert_action_group ("video", null);
	}

	public void save_preferences () {
		if (playbin != null) {
			double volume;
			bool mute;
			playbin.get ("volume", out volume, "mute", out mute);
			settings.set_int (PREF_VIDEO_VOLUME, (int) (volume * 100.0));
			settings.set_boolean (PREF_VIDEO_MUTE, mute);
			settings.set_boolean (PREF_VIDEO_ZOOM_TO_FIT, _zoom_to_fit);
		}
	}

	public void release_resources () {
		if (update_volume_id != 0) {
			Source.remove (update_volume_id);
			update_volume_id = 0;
		}
		if (progress_id != 0) {
			Source.remove (progress_id);
			progress_id = 0;
		}
		if (video_view != null) {
			video_view.release_resources ();
		}
		if (playbin != null) {
			playbin.set_state (Gst.State.NULL);
			wait_playbin_state_change_to_complete ();

			var bus = playbin.get_bus ();
			bus.remove_signal_watch ();

			playbin = null;
		}
	}

	public bool on_scroll (double dx, double dy, Gdk.ModifierType state) {
		switch (scroll_action) {
		case ScrollAction.CHANGE_FILE:
			return window.viewer.on_scroll_change_file (dx, dy);

		case ScrollAction.CHANGE_VOLUME:
			var direction = (dy < 0) ? 1 : -1;
			unowned var adj = builder.get_object ("volume_adjustment") as Gtk.Adjustment;
			adj.value = adj.value + (adj.value * 0.1f * direction);
			break;

		case ScrollAction.CHANGE_CURRENT_TIME:
			var direction = (dy < 0) ? -1 : 1;
			skip (5 * direction);
			break;

		default:
			break;
		}
		return false;
	}

	public bool get_pixel_size (out uint width, out uint height) {
		width = video_width;
		height = video_height;
		return has_video;
	}

	public void focus () {
		if (has_video) {
			video_view.grab_focus ();
		}
		else {
			audio_view.grab_focus ();
		}
	}

	//public async void save_async () throws Error;

	void create_playbin () {
		assert (playbin == null);

		double volume;
		var mute = get_mute_from_settings (out volume);
		playbin = Gst.ElementFactory.make_full ("playbin",
			"volume", volume,
			"mute", mute,
			"force-aspect-ratio", true) as Gst.Pipeline;

		// TODO: check if playbin is null

		// Adjust audio when not playing at normal speed.
		var scaletempo = Gst.ElementFactory.make ("scaletempo");
		if (scaletempo != null) {
			playbin.set ("audio-filter", scaletempo);
		}

		// Rotate according to the orientation tag.
		var autorotate = Gst.ElementFactory.make_full ("videoflip",
			"video-direction", Gst.Video.OrientationMethod.AUTO);
		if (autorotate != null) {
			playbin.set ("video-filter", autorotate);
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
				video_view.paintable = sink_paintable;
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
		if (mute) {
			volume = 0.0;
		}

		// Cubic in [0,1], linear in [1,2]
		double linear_volume;
		if (volume <= 1.0) {
			linear_volume = Math.exp (1.0 / 3.0 * Math.log (volume)); // cube root of volume
		}
		else {
			linear_volume = volume;
		}

		// Update the volume adjustment.
		unowned var adj = builder.get_object ("volume_adjustment") as Gtk.Adjustment;
		SignalHandler.block (adj, volume_changed_id);
		adj.set_value (linear_volume * 100.0);
		SignalHandler.unblock (adj, volume_changed_id);

		// Update the volume button icon.
		// Use the mute icon only when the volume is exactly zero.
		var volume_icon = builder.get_object ("volume_icon") as Gtk.Image;
		var non_mute_icons = VOLUME_ICONS.length - 1;
		var icon_idx = (linear_volume == 0) ? 0 : 1 + ((int) (linear_volume * non_mute_icons)).clamp (0, non_mute_icons - 1);
		volume_icon.set_from_icon_name (VOLUME_ICONS[icon_idx]);
	}

	uint progress_id = 0;

	void update_play_button (Gst.State old_state, Gst.State new_state) {
		unowned var play_button = builder.get_object ("play_button") as Gtk.Button;
		unowned var play_icon = builder.get_object ("play_button_image") as Gtk.Image;
		if ((old_state != Gst.State.PLAYING) && (new_state == Gst.State.PLAYING)) {
			play_icon.set_from_icon_name ("gth-pause-large-symbolic");
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
			play_icon.set_from_icon_name ("gth-play-large-symbolic");
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

	void end_of_stream () {
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
				end_of_stream ();
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
		if (playbin == null) {
			return;
		}
		playbin.seek (rate,	Gst.Format.TIME,
			Gst.SeekFlags.FLUSH | Gst.SeekFlags.ACCURATE,
			Gst.SeekType.SET, time,
			Gst.SeekType.NONE, 0);
	}

	void skip (int seconds) {
		if (playbin == null) {
			return;
		}
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

	void next_frame () {
		if ((playbin == null) || !has_video) {
			return;
		}
		if (_playing) {
			playing = false;
		}
		playbin.send_event (new Gst.Event.step (Gst.Format.BUFFERS,
				1,
				rate,
				true,
				false));
	}

	void set_speed (double value) {
		rate = value;
		skip_to (get_current_time ());
		var action = action_group.lookup_action ("set-speed") as SimpleAction;
		if (action != null) {
			action.set_state (new Variant.string (Util.format_double (rate, "%0.2f")));
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
		action_group = new SimpleActionGroup ();
		window.insert_action_group ("video", action_group);

		var action = new SimpleAction ("decrease-speed", null);
		action.activate.connect ((_action, param) => {
			var idx = get_current_rate_idx ();
			set_speed ((idx > 0) ? Rates[idx - 1] : Rates[0]);
		});
		action_group.add_action (action);

		action = new SimpleAction ("increase-speed", null);
		action.activate.connect ((_action, param) => {
			var idx = get_current_rate_idx ();
			set_speed ((idx < Rates.length - 1) ? Rates[idx + 1] : Rates[Rates.length - 1]);
		});
		action_group.add_action (action);

		action = new SimpleAction ("normal-speed", null);
		action.activate.connect ((_action, param) => {
			set_speed (1.0);
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

		action = new SimpleAction.stateful ("set-speed", VariantType.STRING, new Variant.string (Util.format_double (rate, "%0.2f")));
		action.activate.connect ((_action, param) => {
			set_speed (double.parse (param.get_string ()));
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
			var local_job = window.new_job (_("Saving File"), JobFlags.FOREGROUND);
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

		action = new SimpleAction.stateful ("mute", null, new Variant.boolean (get_mute_from_settings ()));
		action.activate.connect ((action, param) => {
			action.set_state (new Variant.boolean (toggle_mute ()));
		});
		action_group.add_action (action);

		action = new SimpleAction ("next-frame", null);
		action.activate.connect ((action, param) => {
			next_frame ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("copy-frame", null);
		action.activate.connect ((action, param) => copy_frame.begin ());
		action_group.add_action (action);
	}

	async void copy_frame () {
		var local_job = window.new_job (_("Copying to the Clipboard"), JobFlags.FOREGROUND);
		try {
			var was_playing = playing;
			if (was_playing) {
				playing = false;
			}
			var frame = get_current_frame ();
			var bytes = save_png (frame, null, local_job.cancellable);
			unowned var clipboard = window.get_clipboard ();
			var content_provider = new Gdk.ContentProvider.for_bytes ("image/png", bytes);
			if (!clipboard.set_content (content_provider)) {
				throw new IOError.FAILED (_("Could not copy the image to the clipboard"));
			}
			if (was_playing) {
				playing = true;
			}

			var toast = Util.new_literal_toast (_("Copied to the Clipboard"));
			toast.button_label = _("Open");
			toast.action_name = "win.open-clipboard";
			toast.priority = Adw.ToastPriority.HIGH;
			window.add_toast (toast);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	bool toggle_mute () {
		if (playbin == null) {
			return false;
		}
		double volume;
		bool mute;
		playbin.get ("volume", out volume, "mute", out mute);
		mute = !mute;
		if (!mute && (volume == 0)) {
			volume = 1.0;
		}
		playbin.set ("mute", mute, "volume", volume);
		return mute;
	}

	bool get_mute_from_settings (out double volume = null) {
		volume = (double) settings.get_int (PREF_VIDEO_VOLUME) / 100.0;
		return (volume == 0) || settings.get_boolean (PREF_VIDEO_MUTE);
	}

	async void save_screenshot (Cancellable cancellable) throws Error {
		var was_playing = playing;
		if (was_playing) {
			playing = false;
		}

		var screenshot = get_current_frame ();

		// Basename
		var basename = Util.remove_extension (window.viewer.current_file.info.get_display_name ());
		if (basename == null) {
			throw new IOError.FAILED (_("Invalid filename"));
		}

		// Extension
		var screenshot_type = settings.get_string (PREF_VIDEO_SCREENSHOT_TYPE);
		var preferences = app.get_saver_preferences (screenshot_type);
		if (preferences == null) {
			throw new IOError.FAILED (_("Invalid filename"));
		}
		var extension = preferences.get_default_extension ();

		// Destination
		var destination = Gth.Settings.get_file (settings, PREF_VIDEO_SCREENSHOT_LOCATION);
		if (destination == null) {
			destination = Files.get_special_dir (UserDirectory.PICTURES);
			try {
				destination.make_directory_with_parents (cancellable);
			}
			catch (Error error) {
				if (!(error is IOError.EXISTS)) {
					throw error;
				}
			}
			settings.set_string (PREF_VIDEO_SCREENSHOT_LOCATION, destination.get_uri ());
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

		var file_data = new FileData.for_file (file, screenshot_type);
		yield app.image_saver.replace_file (window.monitor_profile, screenshot, file_data, SaveFlags.DEFAULT, cancellable);

		if (was_playing) {
			playing = true;
		}

		// Translators: '%s' is replaced with a filename (do not change it).
		var toast = Util.new_literal_toast (_("Saved %s").printf (file.get_path ()));
		toast.button_label = _("Open");
		toast.action_name = "app.open-new-window";
		toast.action_target = new Variant.string (file.get_uri ());
		toast.timeout = 2;
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

		mediabar.visible = has_video || has_audio;
		if (has_video) {
			view_stack.set_visible_child (video_view);
		}
		else {
			audio_status.gicon = window.viewer.current_file.info.get_icon ();
			audio_status.description = window.viewer.current_file.info.get_display_name ();
			view_stack.set_visible_child (audio_view);
		}
		update_sensitivity ();
		focus ();
	}

	void update_sensitivity () {
		Util.enable_action (action_group, "mute", has_audio);
		Util.enable_action (action_group, "save-screenshot", has_video);
		Util.enable_action (action_group, "next-frame", has_video);
		Util.enable_action (action_group, "zoom-to-fit", has_video);
	}

	void reset_view () {
		mediabar.visible = false;
		view_stack.set_visible_child (empty_view);
	}

	void reset_state () {
		total_time = 0;
		paused = false;
		_playing = false;
		video_width = 0;
		video_height = 0;
		has_audio = false;
		has_video = false;
	}

	Gth.Image? get_current_frame () throws Error {
		// Copied from Showtime
		// https://gitlab.gnome.org/GNOME/showtime/-/blob/1c03149187ab1e20317d8e3dfcf456402090b577/showtime/utils.py
		if (video_view.paintable == null) {
			throw new IOError.FAILED ("No paintable");
		}
		var current_image = video_view.paintable.get_current_image ();
		var width = current_image.get_intrinsic_width ();
		var height = current_image.get_intrinsic_height ();
		var snapshot = new Gtk.Snapshot ();
		current_image.snapshot (snapshot, width, height);
		var node = snapshot.to_node ();
		if (node == null) {
			throw new IOError.FAILED ("No render node");
		}
		var renderer = window.get_renderer ();
		if (renderer == null) {
			throw new IOError.FAILED ("No renderer for this window");
		}
		Graphene.Rect viewport = { { 0, 0 }, { width, height } };
		var texture = renderer.render_texture (node, viewport);
		return new Gth.Image.from_texture (texture);
	}

	construct {
		settings = new GLib.Settings (GTHUMB_VIDEOS_SCHEMA);
		settings.changed.connect ((key) => {
			switch (key) {
			case PREF_VIDEO_SCROLL_ACTION:
				scroll_action = (ScrollAction) settings.get_enum (PREF_VIDEO_SCROLL_ACTION);
				break;
			}
		});
		window = null;
		playbin = null;
		video_view = null;
		_zoom_to_fit = true;
		rate = 1.0;
		loop = false;
		builder = null;
		reset_state ();
	}

	GLib.Settings settings;
	weak Gth.Window window;
	Gst.Pipeline playbin;
	unowned Gtk.Stack view_stack;
	unowned Gth.VideoView video_view;
	unowned Adw.StatusPage audio_view;
	unowned Gtk.Widget empty_view;
	unowned Gth.FolderStatus audio_status;
	bool _zoom_to_fit;
	bool _playing;
	Gtk.Builder builder;
	int64 total_time;
	ulong position_changed_id;
	ulong volume_changed_id;
	double rate;
	bool loop;
	unowned Gtk.Widget mediabar;
	bool paused;
	int video_width;
	int video_height;
	bool has_audio;
	bool has_video;
	ScrollAction scroll_action;
	SimpleActionGroup action_group;

	const double[] Rates = {
		0.25, 0.33, 0.5, 0.75,
		1.0, // Note: Keep NORMAL_RATE_IDX up-to-date
		1.25, 1.5, 1.75, 2.0, 4.0
	};
	const int NORMAL_RATE_IDX = 4;
	const int MAX_ATTEMPTS = 1000;
	const string[] VOLUME_ICONS = {
		"audio-volume-muted-symbolic",
		"audio-volume-low-symbolic",
		"audio-volume-medium-symbolic",
		"audio-volume-high-symbolic",
	};
}
