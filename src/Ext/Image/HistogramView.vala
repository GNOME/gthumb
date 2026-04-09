[GtkTemplate (ui = "/org/gnome/gthumb/ui/histogram-view.ui")]
public class Gth.HistogramView : Gtk.Box {
	public enum DisplayMode {
		ONE_CHANNEL,
		ALL_CHANNELS
	}

	public enum ScaleType {
		LINEAR,
		LOGARITHMIC
	}

	public Histogram histogram {
		get {
			return _histogram;
		}
		set {
			if (changed_id != 0) {
				_histogram.disconnect (changed_id);
				changed_id = 0;
			}
			_histogram = value;
			if (_histogram != null) {
				changed_id = _histogram.changed.connect (() => view.queue_draw ());
			}
		}
	}
	public Channel channel {
		get {
			return _channel;
		}
		set {
			if (_channel != value) {
				_channel = value;
				view.queue_draw ();
			}
		}
	}
	public DisplayMode display_mode {
		get {
			return _display_mode;
		}
		set {
			if (_display_mode != value) {
				_display_mode = value;
				view.queue_draw ();
			}
		}
	}
	public ScaleType scale_type {
		get {
			return _scale_type;
		}
		set {
			if (_scale_type != value) {
				_scale_type = value;
				view.queue_draw ();
			}
		}
	}

	public void set_selection (uchar start, uchar end) {
		selection_start = start;
		selection_end = start;
	}

	[GtkCallback]
	void on_channel_selected (Object obj, ParamSpec param) {
		var selector = obj as Gtk.DropDown;
		if (selector.selected >= Channel.SIZE) {
			display_mode = DisplayMode.ALL_CHANNELS;
		}
		else {
			display_mode = DisplayMode.ONE_CHANNEL;
			channel = (Channel) selector.selected;
		}
	}

	[GtkCallback]
	void on_logarithmic_toggled (Object obj, ParamSpec param) {
		var button = obj as Gtk.ToggleButton;
		scale_type = button.active ? ScaleType.LOGARITHMIC : ScaleType.LINEAR;
	}

	void draw_func (Gtk.DrawingArea area, Cairo.Context cr, int width, int height) {
		draw_grid (cr, width, height);
		if (_histogram != null) {
			if (_display_mode == DisplayMode.ALL_CHANNELS) {
				draw_all_channels (cr, width, height);
			}
			else {
				draw_selected_channel (cr, width, height);
			}
		}
	}

	void draw_grid (Cairo.Context cr, int width, int height) {
		cr.save ();
		cr.set_line_width (0.5);
		cr.set_source_rgba (1.0, 1.0, 1.0, 0.5);
		var step = (double) width / GRID_LINES;
		for (var i = 1; i < GRID_LINES; i++) {
			var x = i * step;
			cr.move_to (x + 0.5, 0);
			cr.line_to (x + 0.5, height);
		}
		cr.stroke ();
		cr.restore ();
	}

	void draw_all_channels (Cairo.Context cr, int width, int height) {
		cr.save ();
		var max = double.max (histogram.get_channel_max (Channel.RED), histogram.get_channel_max (Channel.GREEN));
		max = double.max (max, histogram.get_channel_max (Channel.BLUE));
		if (max > 0.0) {
			max = scaled_value (max);
		}
		else {
			max = 1.0;
		}
		var step = (double) width / 256.0;
		cr.set_line_width (0.5);
		for (var i = 0; i <= 255; i++) {
			var red = _histogram.get_value (Channel.RED, i);
			var green = _histogram.get_value (Channel.GREEN, i);
			var blue = _histogram.get_value (Channel.BLUE, i);

			// Find the channel with minimum value.
			Channel min_channel;
			if ((red <= green) && (red <= blue)) {
				min_channel = Channel.RED;
			}
			else if ((green <= red) && (green <= blue)) {
				min_channel = Channel.GREEN;
			}
			else {
				min_channel = Channel.BLUE;
			}

			// Find the channel with maximum value.
			Channel max_channel;
			if ((red >= green) && (red >= blue)) {
				max_channel = Channel.RED;
			}
			else if ((green >= red) && (green >= blue)) {
				max_channel = Channel.GREEN;
			}
			else {
				max_channel = Channel.BLUE;
			}

			// Find the channel with middle value.
			Channel mid_channel;
			int channel_delta = ((int) max_channel - (int) min_channel).abs ();
			if (channel_delta == 1) {
				if ((max_channel == Channel.RED) || (min_channel == Channel.RED)) {
					mid_channel = Channel.BLUE;
				}
				else {
					mid_channel = Channel.RED;
				}
			}
			else {
				mid_channel = Channel.GREEN;
			}

			// Use the OVER operator for the maximum value.
			cr.set_operator (Cairo.Operator.OVER);
			var rgba = CHANNEL_COLOR[max_channel];
			cr.set_source_rgba (rgba.red, rgba.green, rgba.blue, rgba.alpha);
			var value = _histogram.get_value (max_channel, i);
			var y = (scaled_value (value) * height) / max;
			y = y.clamp (0.0, height);
			cr.rectangle ((i * step) + 0.5, height - y + 0.5, step, y);
			cr.fill ();

			// Use the ADD operator for the middle value.
			cr.set_operator (Cairo.Operator.ADD);
			rgba = CHANNEL_COLOR[mid_channel];
			cr.set_source_rgba (rgba.red, rgba.green, rgba.blue, rgba.alpha);
			value = _histogram.get_value (mid_channel, i);
			y = (scaled_value (value) * height) / max;
			y = y.clamp (0.0, height);
			cr.rectangle ((i * step) + 0.5, height - y + 0.5, step, y);
			cr.fill ();

			// The minimum value is shared by all the channels and is
			// painted in white if inside the selection, otherwise in black.
			cr.set_operator (Cairo.Operator.OVER);
			if (((selection_start > 0) || (selection_end < 255))
				&& (i >= selection_start) && (i <= selection_end))
			{
				cr.set_source_rgb (0.3, 0.3, 0.3);
			}
			else {
				cr.set_source_rgb (1.0, 1.0, 1.0);
			}
			value = _histogram.get_value (min_channel, i);
			y = (scaled_value (value) * height) / max;
			y = y.clamp (0.0, height);
			cr.rectangle ((i * step) + 0.5, height - y + 0.5, step, y);
			cr.fill ();
		}
		cr.restore ();
	}

	void draw_selected_channel (Cairo.Context cr, int width, int height) {
		cr.save ();
		var rgba = CHANNEL_COLOR[_channel];
		cr.set_source_rgba (rgba.red, rgba.green, rgba.blue, rgba.alpha);
		if (_display_mode == DisplayMode.ALL_CHANNELS) {
			cr.set_operator (Cairo.Operator.ADD);
		}
		else {
			cr.set_operator (Cairo.Operator.OVER);
		}
		var max = _histogram.get_channel_max (_channel);
		if (max > 0) {
			max = scaled_value (max);
		}
		else {
			max = 1.0;
		}
		var step = (double) width / 256.0;
		cr.set_line_width (0.5);
		for (var i = 0; i <= 255; i++) {
			var value = _histogram.get_value (_channel, i);
			var y = (scaled_value (value) * height) / max;
			y = y.clamp (0.0, height);
			cr.rectangle ((i * step) + 0.5, height - y + 0.5, step, y);
		}
		cr.fill ();
		cr.restore ();
	}

	inline double scaled_value (double x) {
		return (_scale_type == ScaleType.LOGARITHMIC) ? Math.log (x) : x;
	}

	construct {
		_histogram = null;
		_channel = Channel.VALUE;
		_display_mode = DisplayMode.ONE_CHANNEL;
		_scale_type = ScaleType.LINEAR;
		selection_start = 0;
		selection_end = 255;
		view.set_draw_func (draw_func);
	}

	[GtkChild] unowned Gtk.DrawingArea view;
	Histogram _histogram;
	Channel _channel;
	DisplayMode _display_mode;
	ScaleType _scale_type;
	ulong changed_id;
	uchar selection_start;
	uchar selection_end;

	const int GRID_LINES = 5;
	const Gdk.RGBA CHANNEL_COLOR[Channel.SIZE] = {
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		{ 0.68f, 0.18f, 0.19f, 1.0f }, // #AF2E31
		{ 0.33f, 0.78f, 0.30f, 1.0f }, // #55C74D
		{ 0.13f, 0.54f, 0.8f, 1.0f }, // #238ACC
		{ 0.5f, 0.5f, 0.5f, 1.0f },
	};
}
