[GtkTemplate (ui = "/org/gnome/gthumb/ui/slideshow-preferences.ui")]
public class Gth.SlideshowPreferences : Adw.NavigationPage {
	construct {
		settings = new GLib.Settings (GTHUMB_SLIDESHOW_SCHEMA);
		constructing = true;
		automatic.enable_expansion = settings.get_boolean (PREF_SLIDESHOW_AUTOMATIC);
		automatic.expanded = automatic.enable_expansion;
		random_order.active = settings.get_boolean (PREF_SLIDESHOW_RANDOM_ORDER);
		loop.active = settings.get_boolean (PREF_SLIDESHOW_LOOP);
		delay.value = settings.get_int (PREF_SLIDESHOW_DELAY);
		constructing = false;
	}

	[GtkCallback]
	void on_automatic_enabled (Object obj, ParamSpec param) {
		if (constructing) {
			return;
		}
		settings.set_boolean (PREF_SLIDESHOW_AUTOMATIC, automatic.enable_expansion);
	}

	[GtkCallback]
	void on_delay_changed (Gtk.Adjustment adj) {
		if (constructing) {
			return;
		}
		settings.set_int (PREF_SLIDESHOW_DELAY, (int) delay.value);
	}

	[GtkCallback]
	void on_random_order_changed (Object obj, ParamSpec param) {
		if (constructing) {
			return;
		}
		settings.set_boolean (PREF_SLIDESHOW_RANDOM_ORDER, random_order.active);
	}

	[GtkCallback]
	void on_loop_changed (Object obj, ParamSpec param) {
		if (constructing) {
			return;
		}
		settings.set_boolean (PREF_SLIDESHOW_LOOP, loop.active);
	}

	GLib.Settings settings;
	bool constructing;

	[GtkChild] unowned Adw.ExpanderRow automatic;
	[GtkChild] unowned Gtk.Adjustment delay;
	[GtkChild] unowned Adw.SwitchRow random_order;
	[GtkChild] unowned Adw.SwitchRow loop;
}
