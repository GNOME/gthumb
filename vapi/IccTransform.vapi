using GLib;

namespace Gth {
	[PointerType]
	public struct CmsTransform {
	}

	[CCode (cheader_filename = "lib/gth-icc-profile.h", type_id = "gth_icc_transform_get_type ()")]
	public class IccTransform : Object {
		public IccTransform (CmsTransform transform);
		public IccTransform.from_profiles (IccProfile from_profile, IccProfile to_profile);
		public CmsTransform get_transform ();
	}
}
