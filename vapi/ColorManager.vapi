using GLib;

namespace Gth {
	[CCode (cheader_filename = "gth-color-manager.h", type_id = "gth_color_manager_get_type ()")]
	public class ColorManager : Object {
		public signal void changed ();
		public async IccProfile get_profile_async (string monitor_name, Cancellable cancellable);
		public IccTransform get_transform (IccProfile from_profile, IccProfile to_profile);
	}
}
