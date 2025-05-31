using GLib;

namespace Gth {
	[CCode (cheader_filename = "lib/gth-image.h", type_id = "gth_image_get_type ()")]
	public class Image : Object {
		public Image (uint width, uint height);
		public Image dup ();
		public void copy_metadata (Image destination);
		public void copy_pixels (Image destination);
		public unowned uint8[] get_pixels (out size_t size, out int row_stride);
		public uint get_width ();
		public uint get_height ();
		public size_t get_size ();
		public void set_has_alpha (bool has_alpha);
		public bool get_has_alpha (out bool has_alpha);
		public void set_original_size (uint width, uint height);
		public bool get_original_size (out uint width, out uint height);
		public bool get_original_image_size (out uint width, out uint height);
		public virtual bool get_is_zoomable ();
		public virtual bool set_zoom (double zoom, out uint original_width, out uint original_height);
		public Gdk.Texture get_gdk_texture ();
		public void set_icc_profile (IccProfile profile);
		public unowned IccProfile? get_icc_profile ();
		public bool apply_icc_profile (ColorManager color_manager, IccProfile profile, Cancellable cancellable);
		public async bool apply_icc_profile_async (ColorManager color_manager, IccProfile profile, Cancellable cancellable) throws Error;
		public Image? resize_if_larger (uint size, ScaleFilter quality, Cancellable cancellable);
		public async Image? resize_if_larger_async (uint size, ScaleFilter quality, Cancellable cancellable) throws Error;
	}
}
