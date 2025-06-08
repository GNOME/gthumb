public class Gth.CatalogSearch : Gth.Catalog {
	public override void update_file_info (File file, FileInfo info) {
		base.update_file_info (file, info);
		info.set_content_type ("gthumb/search");
		info.set_symbolic_icon (new ThemedIcon ("search-symbolic"));
	}
}
