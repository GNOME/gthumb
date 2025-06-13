using GLib;

namespace Gth {
	[CCode (cheader_filename = "lib/gth-icc-profile.h")]
	[PointerType]
	public struct CmsProfile {
	}

	[CCode (cheader_filename = "lib/gth-icc-profile.h", type_id = "gth_icc_profile_get_type ()")]
	public class IccProfile : Object {
		public IccProfile (string id, CmsProfile profile);
		[CCode (cname = "gth_icc_profile_new_srgb")]
		public IccProfile.sRGB ();
		[CCode (cname = "gth_icc_profile_new_adobergb")]
		public IccProfile.AdobeRGB ();
		[CCode (cname = "gth_icc_profile_new_srgb_with_gamma")]
		public IccProfile.sRGB_with_gamma (double gamma);
		public unowned string get_id ();
		public unowned string get_description ();
		public static bool id_is_unknown (string id);
		public CmsProfile get_profile ();
		public bool equal (IccProfile other);
	}
}
