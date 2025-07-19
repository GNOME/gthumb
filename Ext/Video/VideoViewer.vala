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
				playbin.set_state (_playing ? Gst.State.PLAYING : Gst.State.PAUSED);
				wait_playbin_state_change_to_complete ();
			}
		}
	}

	construct {
		settings = new GLib.Settings (GTHUMB_VIDEO_SCHEMA);
		window = null;
		playbin = null;
		picture = null;
		_zoom_to_fit = true;
		_playing = false;
		total_time = 0;
		rate = 1.0;
		loop = false;
	}

	public async void load (FileData file_data) throws Error {
		if (playbin == null)
			return;
		playbin.set_state (Gst.State.NULL);
		playbin.set ("uri", file_data.file.get_uri ());
		playbin.set_state (Gst.State.PLAYING);
		wait_playbin_state_change_to_complete ();
		_playing = true;
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

		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/mediabar.ui");
		mediabar_revealer = builder.get_object ("mediabar_revealer") as Gtk.Revealer;
		window.viewer.add_viewer_overlay (mediabar_revealer);

		var click_events = new Gtk.GestureClick ();
		click_events.button = Gdk.BUTTON_PRIMARY;
		click_events.pressed.connect ((n_press, x, y) => {
			playing = !playing;
		});
		picture.add_controller (click_events);

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((event, x, y) => {
			if (((last_x - x).abs () < MOTION_THRESHOLD)
				|| ((last_y - y).abs () < MOTION_THRESHOLD))
			{
				return;
			}
			last_x = x;
			last_y = y;
			if (!mediabar_revealer.reveal_child) {
				mediabar_revealer.reveal_child = true;
				hide_mediabar_after_timeout ();
			}
		});
		picture.add_controller (motion_events);

		unowned var mediabar = builder.get_object ("mediabar") as Gtk.Widget;
		motion_events = new Gtk.EventControllerMotion ();
		motion_events.enter.connect (() => {
			cancel_hide_mediabar_timeout ();
			on_mediabar = true;
		});
		motion_events.leave.connect (() => {
			on_mediabar = false;
			hide_mediabar_after_timeout ();
		});
		mediabar.add_controller (motion_events);

		unowned var volume_button = builder.get_object ("volume_button") as Gtk.ScaleButton;
		volume_button.notify["active"].connect (() => {
			set_always_show_mediabar (volume_button.active);
		});

		unowned var volume_popover = volume_button.get_popup () as Gtk.Popover;
		if (volume_popover != null) {
			volume_popover.set_position (Gtk.PositionType.TOP);
		}

		unowned var position_button = builder.get_object ("position_button") as Gtk.MenuButton;
		position_button.notify["active"].connect (() => {
			set_always_show_mediabar (position_button.active);
		});

		unowned var adj = builder.get_object ("position_adjustment") as Gtk.Adjustment;
		position_changed_id = adj.value_changed.connect (() => {
			unowned var local_adj = builder.get_object ("position_adjustment") as Gtk.Adjustment;
			var current_time = (int64) (local_adj.get_value () / 100.0 * total_time);
			playbin.seek (rate,
				Gst.Format.TIME,
				Gst.SeekFlags.FLUSH | Gst.SeekFlags.ACCURATE,
				Gst.SeekType.SET,
				current_time,
				Gst.SeekType.NONE,
				0);
			var text = Lib.format_duration_for_display (current_time / 1000000);
			unowned var label = builder.get_object ("label_position") as Gtk.Label;
			label.set_text (text);
		});

		create_playbin ();
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
		if (hide_mediabar_id != 0) {
			Source.remove (hide_mediabar_id);
			hide_mediabar_id = 0;
		}
		window.viewer.remove_viewer_overlay (builder.get_object ("mediabar_revealer") as Gtk.Widget);
		window.viewer.viewer_container.remove_css_class ("video-view");
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
			"force-aspect-ratio", true);

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
		var adj_value = volume;
		if (volume <= 1.0) {
			adj_value = Math.exp (1.0 / 3.0 * Math.log (volume)); // cube root of volume
		}
		// TODO: update volumne adjustment.
	}

	uint progress_id = 0;

	void update_play_button (Gst.State old_state, Gst.State new_state) {
		unowned var play_button = builder.get_object ("play_button") as Gtk.Button;
		unowned var play_icon = builder.get_object ("play_button_image") as Gtk.Image;
		if ((old_state != Gst.State.PLAYING) && (new_state == Gst.State.PLAYING)) {
			play_icon.set_from_icon_name ("media-playback-pause-symbolic");
			play_button.set_tooltip_text (_("Pause"));
			update_playback_info ();
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
			play_icon.set_from_icon_name ("media-playback-start-symbolic");
			play_button.set_tooltip_text (_("Play"));
			update_playback_info ();
		}
	}

	void update_playback_info () {
		// TODO: show the playback speed somewhere.
		// var info = "@%2.2f".printf (rate);
		// viewer->current_file.info.set_attribute_string ("gthumb::statusbar-extra-info", info);
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
			var text = Lib.format_duration_for_display (total_time / 1000000, out hours, out minutes);
			unowned var label = builder.get_object ("label_duration") as Gtk.Label;
			label.set_text (text);
			max_chars = (hours > 0 ? Util.get_digits (hours) + 1 /* ':' */ : 0)
				+ Util.get_digits (minutes) + 3; // ':00'
			label.width_chars = max_chars;
		}
		unowned var adj = builder.get_object ("position_adjustment") as Gtk.Adjustment;
		SignalHandler.block (adj, position_changed_id);
		adj.set_value ((total_time > 0) ? ((double) current_time / total_time) * 100.0 : 0.0);
		SignalHandler.unblock (adj, position_changed_id);

		var text = Lib.format_duration_for_display (current_time / 1000000);
		unowned var label = builder.get_object ("label_position") as Gtk.Label;
		label.set_text (text);
		if (max_chars > 0) {
			label.width_chars = max_chars;
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
			break;

		case Gst.MessageType.STATE_CHANGED:
			var old_state = Gst.State.NULL;
			var new_state = Gst.State.NULL;
			var pending_state = Gst.State.NULL;
			msg.parse_state_changed (out old_state, out new_state, out pending_state);
			if (old_state == new_state)
				break;
			_playing = (new_state == Gst.State.PLAYING);
			update_current_position_bar ();
			if ((old_state == Gst.State.NULL)
				&& (new_state == Gst.State.READY)
				&& (pending_state != Gst.State.PAUSED))
			{
				//update_stream_info (self);
				//gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (self));
				//gth_viewer_page_file_loaded (GTH_VIEWER_PAGE (self), self->priv->file_data, self->priv->updated_info, TRUE);
			}
			if ((old_state == Gst.State.READY) && (new_state == Gst.State.PAUSED)) {
				//update_stream_info (self);
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
				playbin.seek (rate,
					Gst.Format.TIME,
					Gst.SeekFlags.FLUSH | Gst.SeekFlags.ACCURATE,
					Gst.SeekType.SET,
					0,
					Gst.SeekType.NONE,
					0);
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

	void hide_mediabar_after_timeout () {
		if (hide_mediabar_id != 0) {
			Source.remove (hide_mediabar_id);
		}
		hide_mediabar_id = Timeout.add_seconds (1, () => {
			hide_mediabar_id = 0;
			if (!on_mediabar && !always_show_mediabar) {
				mediabar_revealer.reveal_child = false;
			}
			return Source.REMOVE;
		});
	}

	void cancel_hide_mediabar_timeout () {
		if (hide_mediabar_id != 0) {
			Source.remove (hide_mediabar_id);
			hide_mediabar_id = 0;
		}
	}

	void set_always_show_mediabar (bool value) {
		always_show_mediabar = value;
		if (always_show_mediabar) {
			cancel_hide_mediabar_timeout ();
		}
		else {
			hide_mediabar_after_timeout ();
		}
	}

	GLib.Settings settings;
	weak Gth.Window window;
	Gst.Element playbin;
	Gtk.Picture picture;
	bool _zoom_to_fit;
	bool _playing;
	Gtk.Builder builder;
	int64 total_time;
	ulong position_changed_id;
	double rate;
	bool loop;
	unowned Gtk.Revealer mediabar_revealer;
	uint hide_mediabar_id = 0;
	double last_x = 0.0;
	double last_y = 0.0;
	bool always_show_mediabar = false;
	bool on_mediabar = false;

	const double MOTION_THRESHOLD = 1.0;
}
