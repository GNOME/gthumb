public class Gth.CatalogSearch : Gth.Catalog {
	public GenericArray<SearchSource> sources;
	public TestExpr? test;

	public CatalogSearch () {
		sources = new GenericArray<SearchSource>();
		test = null;
	}

	public override void update_file_info (File file, FileInfo info) {
		base.update_file_info (file, info);
		info.set_content_type ("gthumb/search");
		info.set_symbolic_icon (new ThemedIcon ("search-symbolic"));
	}

	public override void load_doc (Dom.Document doc) {
		base.load_doc (doc);
		sources.length = 0;
		test = null;
		foreach (unowned var child in doc.first_child) {
			switch (child.tag_name) {
			case "folder":
				// For backward compatibility, new searches use the 'sources' element.
				var folder = File.new_for_uri (child.get_attribute ("uri"));
				var recursive = child.get_attribute ("recursive") == "true";
				sources.add (new SearchSource (folder, recursive));
				break;

			case "tests":
				test = new TestExpr ();
				test.load_from_element (child);
				break;

			case "sources":
				foreach (unowned var node in child.first_child) {
					if (node.tag_name == "source") {
						var folder = File.new_for_uri (node.get_attribute ("uri"));
						var recursive = node.get_attribute ("recursive") == "true";
						sources.add (new SearchSource (folder, recursive));
					}
				}
				break;
			}
		}
	}
}

[Compact]
public class Gth.SearchSource {
	public File folder;
	public bool recursive;

	public SearchSource (File _folder, bool _recursive) {
		folder = _folder;
		recursive = _recursive;
	}
}
