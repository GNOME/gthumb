using GLib;

namespace Gth {
	[CCode (cheader_filename = "lib/gth-image.h", type_id = "gth_image_get_type ()")]
	public class Image : Object {
		public Image (uint width, uint height);
		public Image.from_texture (Gdk.Texture texture);
		public Image dup ();
		public void copy_metadata (Image destination);
		public void copy_pixels (Image destination);
		[CCode (array_length_cname = "size", array_length_type = "size_t", array_length_pos = 1.1)]
		public unowned uint8[] get_pixels ();
		public uint get_row_stride ();
		public uint get_width ();
		public uint width {
			[CCode (cname = "gth_image_get_width")]
			get;
		}
		public uint get_height ();
		public uint height {
			[CCode (cname = "gth_image_get_height")]
			get;
		}
		public size_t get_size ();
		public bool get_is_empty ();
		public void set_has_alpha (bool has_alpha);
		public bool get_has_alpha (out bool has_alpha);
		public void get_natural_size (out uint width, out uint height);
		public void set_attribute (string key, string? value);
		public bool remove_attribute (string key);
		public unowned string get_attribute (string key);
		public FileInfo info {
			[CCode (cname = "gth_image_get_info")] get;
			[CCode (cname = "gth_image_set_info")] set;
		}

		public void copy_from_rgba_big_endian (uint8* data, bool with_alpha, int row_stride);
		//public const Gdk.MemoryFormat MEMORY_FORMAT;

		public Gdk.Texture get_texture ();
		public Gdk.Texture? get_texture_for_rect (uint x, uint y, uint width, uint height, uint frame_index = 0);
		public Image get_subimage (uint x, uint y, uint width, uint height);
		public bool get_rgba (uint x, uint y, out uint8 red, out uint8 green, out uint8 blue, out uint8 alpha);

		public virtual bool get_is_scalable ();
		public virtual Cairo.Surface? get_scaled_texture (double factor, uint x, uint y, uint width, uint height);

		public bool get_is_animated ();
		public uint get_frames ();
		public bool get_frame_at (ref ulong time, out uint frame_index);
		public bool next_frame (ref uint frame_index);

		public void set_icc_profile (IccProfile profile);
		public unowned IccProfile? get_icc_profile ();
		public bool has_icc_profile ();
		public bool apply_icc_profile (ColorManager color_manager, IccProfile profile, Cancellable cancellable);
		public async bool apply_icc_profile_async (ColorManager color_manager, IccProfile profile, Cancellable cancellable) throws Error;

		public void fill_vertical (Image pattern, Fill fill);

		public Image? resize (uint size, ResizeFlags flags, ScaleFilter quality, Cancellable? cancellable = null);
		public Image? resize_to (uint width, uint height, ScaleFilter quality, Cancellable? cancellable = null);
		public async Image? resize_async (uint size, ResizeFlags flags, ScaleFilter quality, Cancellable cancellable) throws Error;
		public Image? apply_transform (Gth.Transform transform, Cancellable cancellable);
		public Image? cut (uint x, uint y, uint width, uint height, Cancellable cancellable);

		public void apply_value_map (long[,] map);
		public void stretch_histogram (double crop_size);
		public void equalize_histogram (bool linear);
		public void gamma_correction (double gamma);
		public void adjust_brightness (double amount);
		public void adjust_contrast (double amount);
		public void grayscale (double red_weight, double green_weight, double blue_weight, double amount);
		public void grayscale_saturation (double amount);
		public bool sharpen (double amount, double radius, double threshold, Cancellable cancellable);
	}
}
