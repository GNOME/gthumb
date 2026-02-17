public class Gth.MonitorProfile {
	public MonitorProfile (Gtk.Window _window) {
		window = _window;
		color_profile = null;
	}

	public void get_geometry (out int width, out int height) {
		unowned var display = window.get_display ();
		unowned var monitor = display.get_monitor_at_surface (window.get_surface ());
		width = monitor.geometry.width;
		height = monitor.geometry.height;
	}

	public async IccProfile get_color_profile (Cancellable cancellable) throws Error {
		if (color_profile == null) {
			yield update_color_profile (cancellable);
		}
		return color_profile;
	}

	async void update_color_profile (Cancellable cancellable) throws Error {
		try {
			unowned var display = window.get_display ();
			unowned var monitor = display.get_monitor_at_surface (window.get_surface ());
			if (monitor == null) {
				throw new IOError.FAILED ("No monitor");
			}
			color_profile = yield app.color_manager.get_profile_async (monitor.description, cancellable);
		}
		catch (Error error) {
			if (error is IOError.CANCELLED) {
				throw error;
			}
		}
		if (color_profile == null) {
			// Use sRGB by default.
			color_profile = new Gth.IccProfile.sRGB ();
		}
	}

	public async void apply_color_profile (Gth.Image image, FileInfo info, Cancellable cancellable, bool apply_icc_profile = true) throws Error {
		try {
			if (color_profile == null) {
				yield update_color_profile (cancellable);
			}
			if (color_profile == null) {
				throw new IOError.FAILED ("Monitor profile not available");
			}
			if (image.get_icc_profile () == color_profile) {
				return;
			}
			if (image.has_icc_profile ()) {
				if (apply_icc_profile) {
					yield image.apply_icc_profile_async (app.color_manager, color_profile, cancellable);
					unowned var profile_name = image.get_attribute ("Private::ColorProfile");
					if (profile_name != null) {
						info.set_attribute_string (PrivateAttribute.LOADED_IMAGE_COLOR_PROFILE, profile_name);
					}
				}
			}
			else {
				// This allows to convert the image to sRGB before saving.
				image.set_icc_profile (color_profile);
			}
		}
		catch (Error error) {
			if (error is IOError.CANCELLED) {
				throw error;
			}
			// stdout.printf ("> apply profile error: %s\n", error.message);
		}
	}

	weak Gtk.Window window;
	IccProfile color_profile;
}
