public class Gth.ColorSelector : Object {
	public signal void trying (Gdk.RGBA new_color);

	public async Gdk.RGBA? select_color (Gtk.Window? parent, Gdk.RGBA? current_color = null, Cancellable? cancellable = null) throws Error {
		callback = select_color.callback;
		dialog = new ColorSelectorDialog (parent, current_color);
		dialog.changed_color.connect (() => trying (dialog.get_color ()));
		dialog.selected.connect (() => {
			result = dialog.selected_color.copy ();
			dialog.close ();
		});
		dialog.close_request.connect (() => {
			if (callback != null) {
				Idle.add ((owned) callback);
				callback = null;
			}
			return false;
		});
		if (cancellable != null) {
			cancelled_event = cancellable.cancelled.connect (() => {
				dialog.close ();
			});
		}
		dialog.present ();
		yield;
		if (cancelled_event != 0) {
			cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (result == null) {
			throw new IOError.FAILED ("No color selected.");
		}
		return result;
	}

	SourceFunc callback = null;
	ColorSelectorDialog dialog = null;
	ulong cancelled_event = 0;
	Gdk.RGBA? result = null;
}

class Gth.ColorSelectorDialog : Gtk.Window {
	public signal void selected ();
	public signal void changed_color ();
	public Gdk.RGBA selected_color;

	public ColorSelectorDialog (Gtk.Window? _parent = null, Gdk.RGBA? _selected_color = null) {
		title = _("Color");
		default_width = 700;
		resizable = true;
		if (_parent != null)
			transient_for = _parent;
		if (_selected_color != null)
			selected_color = _selected_color;
		else
			selected_color = { 0.5f, 0.5f, 0.5f, 1f };

		var headerbar = new Gtk.HeaderBar ();
		headerbar.show_title_buttons = false;
		titlebar = headerbar;

		var button_group = new Gtk.SizeGroup (Gtk.SizeGroupMode.HORIZONTAL);

		var button = new Gtk.Button.with_mnemonic (_("_Cancel"));
		button_group.add_widget (button);
		button.clicked.connect (() => close ());
		headerbar.pack_start (button);

		button = new Gtk.Button.with_mnemonic (_("_Select"));
		button_group.add_widget (button);
		button.add_css_class ("suggested-action");
		set_default_widget (button);
		button.clicked.connect (() => selected ());
		headerbar.pack_end (button);

		var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
		main_box.add_css_class ("dialog-content");
		child = main_box;

		var grid = new Gtk.Grid ();
		grid.add_css_class ("option-grid");
		main_box.append (grid);

		int row = 0;

		void add_row (string? field, Gtk.Widget widget, Gtk.Widget? entry = null) {
			if (field != null) {
				var label = new Gtk.Label.with_mnemonic (field);
				label.xalign = 1.0f;
				label.mnemonic_widget = widget;
				grid.attach (label, 0, row);
			}

			widget.hexpand = true;
			widget.valign = Gtk.Align.CENTER;
			grid.attach (widget, 1, row);

			if (entry != null)
				grid.attach (entry, 2, row);

			row++;
		}

		bool updating_colors = false;

		void changed_color_range (Gth.ColorRange color_range) {
			if (!updating_colors) {
				updating_colors = true;
				set_color (color_range.get_color (), color_range.range_type);
				updating_colors = false;
			}
		}

		void add_range_row (string field, Gth.ColorRange color_range) {
			color_range.changed.connect ((obj) => changed_color_range (obj));
			var spin_button = new Gtk.SpinButton (color_range.adjustment, 1.0, 1);
			add_row (field, color_range, spin_button);
		}

		// Color Row

		color_preview = new Gth.ColorRange (Gth.ColorRangeType.PREVIEW, selected_color);
		color_preview.tooltip_text = _("Selected Color");

		var preview_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 10);
		preview_box.append (color_preview);
		if (_selected_color != null) {
			var original_color = new Gth.ColorRange (Gth.ColorRangeType.COLOR, selected_color);
			original_color.changed.connect ((obj) => changed_color_range (obj));
			original_color.tooltip_text = _("Reset Original Color");
			preview_box.append (original_color);
		}

		hex_input = new Gtk.Entry ();
		hex_input.set_text (Gth.Color.rgba_to_hexcode (selected_color));
		hex_input.activate.connect (() => {
			var color = Gdk.RGBA ();
			if (color.parse (hex_input.get_text ())) {
				set_color (color);
			}
		});
		add_row (_("_Color:"), hex_input, preview_box);

		// Red, Green, Blue, Alpha

		var separator = new Gtk.Separator (Gtk.Orientation.HORIZONTAL);
		separator.add_css_class ("option-separator");
		grid.attach (separator, 0, row++, 3, 1);

		red_range = new Gth.ColorRange (Gth.ColorRangeType.RED, selected_color);
		add_range_row (_("_Red:"), red_range);

		green_range = new Gth.ColorRange (Gth.ColorRangeType.GREEN, selected_color);
		add_range_row (_("_Green:"), green_range);

		blue_range = new Gth.ColorRange (Gth.ColorRangeType.BLUE, selected_color);
		add_range_row (_("_Blue:"), blue_range);

		alpha_range = new Gth.ColorRange (Gth.ColorRangeType.ALPHA, selected_color);
		add_range_row (_("_Transparency:"), alpha_range);

		// Hue, Saturation, Brightness

		separator = new Gtk.Separator (Gtk.Orientation.HORIZONTAL);
		separator.add_css_class ("option-separator");
		grid.attach (separator, 0, row++, 3, 1);

		hue_range = new Gth.ColorRange (Gth.ColorRangeType.HUE, selected_color);
		add_range_row (_("_Hue:"), hue_range);

		saturation_range = new Gth.ColorRange (Gth.ColorRangeType.SATURATION, selected_color);
		add_range_row (_("_Saturation:"), saturation_range);

		brightness_range = new Gth.ColorRange (Gth.ColorRangeType.BRIGHTNESS, selected_color);
		add_range_row (_("_Brightness:"), brightness_range);

		add_binding_signal (Gdk.Key.Escape, 0, "cancel", null, null);
	}

	[Signal (action=true)]
	public signal void cancel () {
		close ();
	}

	public void set_color (Gdk.RGBA new_color, Gth.ColorRangeType changed_type = Gth.ColorRangeType.COLOR) {
		if (changed_type == Gth.ColorRangeType.ALPHA) {
			new_color = {
				selected_color.red,
				selected_color.green,
				selected_color.blue,
				new_color.alpha // Change transparency
			};
		}
		else if (changed_type != Gth.ColorRangeType.COLOR) {
			new_color.alpha = selected_color.alpha;
		}
		color_preview.set_color (new_color);
		hex_input.set_text (Gth.Color.rgba_to_hexcode (new_color));
		selected_color = new_color;
		if (changed_type != Gth.ColorRangeType.RED)
			red_range.set_color (new_color);
		if (changed_type != Gth.ColorRangeType.GREEN)
			green_range.set_color (new_color);
		if (changed_type != Gth.ColorRangeType.BLUE)
			blue_range.set_color (new_color);
		if (changed_type != Gth.ColorRangeType.ALPHA)
			alpha_range.set_color (new_color);
		if (changed_type != Gth.ColorRangeType.HUE)
			hue_range.set_color (new_color);
		if (changed_type != Gth.ColorRangeType.SATURATION)
			saturation_range.set_color (new_color);
		if (changed_type != Gth.ColorRangeType.BRIGHTNESS)
			brightness_range.set_color (new_color);
		changed_color ();
	}

	public new Gdk.RGBA get_color () {
		return selected_color;
	}

	const string CSS = """
	.dialog-content {
		margin: 25px;
	}
	.option-grid {
		border-spacing: 15px 10px;
	}
	.option-separator {
		margin: 5px 0;
		color: transparent;
		background-color: transparent;
	}
	""";

	Gtk.CssProvider css_provider;

	public override void map () {
		css_provider = new Gtk.CssProvider ();
		Gtk.StyleContext.add_provider_for_display (Gdk.Display.get_default (), css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);
		css_provider.load_from_string (CSS);
		base.map ();
	}

	public override void unmap () {
		base.unmap ();
		css_provider = null;
	}

	Gth.ColorRange color_preview;
	Gth.ColorRange hue_range;
	Gth.ColorRange saturation_range;
	Gth.ColorRange brightness_range;
	Gth.ColorRange red_range;
	Gth.ColorRange green_range;
	Gth.ColorRange blue_range;
	Gth.ColorRange alpha_range;
	Gtk.Entry hex_input;
}
