namespace Gth.FileUtil {
    public static Gdk.ContentProvider[] get_content_providers_for_files (Gth.GenericList<File> files) {
		var providers = new Gdk.ContentProvider[] {};
		if (files.length () > 0) {
			var text = new StringBuilder ();
			var uri_list = new SList<File> ();
			foreach (unowned var file in files) {
				if (text.len > 0) {
					text.append ("\n");
				}
				if (file.get_uri_scheme () == "file") {
					text.append (file.get_path ());
				}
				else {
					text.append (file.get_uri ());
				}
				uri_list.append (file);
			}
			var text_provider = new Gdk.ContentProvider.for_value (text.str);
			providers += text_provider;
			var uri_provider = new Gdk.ContentProvider.for_value ((Gdk.FileList) uri_list);
			providers += uri_provider;
		}
		return providers;
	}
}
