public class Gth.DesktopBackground {
	public DesktopBackground (Gth.Window _window) {
		window = _window;
	}

	public async void set_file (Gth.FileData file_data, Cancellable cancellable) throws Error {
		if (!Util.content_type_is_image (file_data.get_content_type ())) {
			throw new IOError.FAILED (_("File type not supported"));
		}
		if (!file_data.file.is_native ()) {
			throw new IOError.FAILED (_("File type not supported"));
		}
		settings = new GLib.Settings (DESKTOP_BACKGROUND_SCHEMA);
		old_wallpaper = Wallpaper.from_settings (settings);
		new_wallpaper = Wallpaper.for_file (file_data.file);
		yield set_wallpaper (new_wallpaper, file_data.get_content_type (), cancellable);
	}

	public async void undo (Cancellable cancellable) throws Error {
		settings.set_string (DesktopBackground.LIGHT_URI, old_wallpaper.light.get_uri ());
		settings.set_string (DesktopBackground.DARK_URI, old_wallpaper.dark.get_uri ());
		settings.set_enum (DesktopBackground.OPTIONS, old_wallpaper.style);
	}

	async void set_wallpaper (Wallpaper wallpaper, string content_type, Cancellable cancellable) throws Error {
		if (wallpaper.style == WallpaperStyle.NONE) {
			var file_data = yield FileData.read_metadata (wallpaper.file, "Frame::Width,Frame::Height", cancellable);
			int image_width = file_data.info.get_attribute_int32 ("Frame::Width");
			int image_height = file_data.info.get_attribute_int32 ("Frame::Height");

			if ((image_width) > 0 && (image_height > 0)) {
				int monitor_width, monitor_height;
				window.get_monitor_geometry (out monitor_width, out monitor_height);

				double scale_factor = double.max (
					(double) monitor_width / image_width,
					(double) monitor_height / image_height);

				// Allow SVG images to be zoomed more.
				double max_scale_factor = (content_type == "image/svg+xml") ? 5.0 : 2.0;
				stdout.printf ("> scale_factor: %f (max: %f)\n", scale_factor, max_scale_factor);
				wallpaper.style = (scale_factor <= max_scale_factor) ? WallpaperStyle.ZOOM : WallpaperStyle.WALLPAPER;
			}
			else {
				wallpaper.style = (content_type == "image/svg+xml") ? WallpaperStyle.ZOOM : WallpaperStyle.WALLPAPER;
			}
		}
		settings.set_string (DesktopBackground.LIGHT_URI, wallpaper.light.get_uri ());
		settings.set_string (DesktopBackground.DARK_URI, wallpaper.dark.get_uri ());
		settings.set_enum (DesktopBackground.OPTIONS, wallpaper.style);

		var toast = Util.new_literal_toast (_("Background Updated"));
		toast.button_label = _("Undo");
		toast.action_name = "win.undo-desktop-background";
		toast.priority = Adw.ToastPriority.HIGH;
		toast.timeout = 0;
		window.add_toast (toast);
	}

	Window window;
	Wallpaper old_wallpaper;
	Wallpaper new_wallpaper;
	GLib.Settings settings;

	const string DESKTOP_BACKGROUND_SCHEMA = "org.gnome.desktop.background";
	public const string LIGHT_URI = "picture-uri";
	public const string DARK_URI = "picture-uri-dark";
	public const string OPTIONS = "picture-options";
}

struct Gth.Wallpaper {
	File? file;
	File? light;
	File? dark;
	WallpaperStyle style;

	public Wallpaper.from_settings (GLib.Settings settings) {
		file = null;
		light = File.new_for_uri (settings.get_string (DesktopBackground.LIGHT_URI));
		dark = File.new_for_uri (settings.get_string (DesktopBackground.DARK_URI));
		style = (WallpaperStyle) settings.get_enum (DesktopBackground.OPTIONS);
	}

	public Wallpaper.for_file (File _file) {
		file = _file;
		light = file;
		dark = file;
		style = WallpaperStyle.NONE;
	}
}

enum Gth.WallpaperStyle {
	NONE,
	WALLPAPER,
	CENTERED,
	SCALED,
	STRETCHED,
	ZOOM,
	SPANNED
}
