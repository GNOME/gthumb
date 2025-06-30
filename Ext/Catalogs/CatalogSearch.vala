public class Gth.CatalogSearch : Gth.Catalog {
	public GenericList<SearchSource> sources;
	public TestExpr? test;

	public CatalogSearch () {
		sources = new GenericList<SearchSource>();
		test = null;
	}

	public override void update_file_info (File file, FileInfo info) {
		base.update_file_info (file, info);
		info.set_content_type ("gthumb/search");
		info.set_symbolic_icon (new ThemedIcon ("search-symbolic"));
	}

	public override void load_doc (Dom.Document doc) {
		base.load_doc (doc);
		sources.model.remove_all ();
		test = null;
		foreach (unowned var child in doc.first_child) {
			switch (child.tag_name) {
			case "folder":
				// For backward compatibility, new searches use the 'sources' element.
				var folder = File.new_for_uri (child.get_attribute ("uri"));
				var recursive = child.get_attribute ("recursive") == "true";
				sources.model.append (new SearchSource (folder, recursive));
				break;

			case "tests":
				test = new TestExpr ();
				test.load_from_element (child);
				break;

			case "sources":
				foreach (unowned var node in child) {
					if (node.tag_name == "source") {
						var folder = File.new_for_uri (node.get_attribute ("uri"));
						var recursive = node.get_attribute ("recursive") == "true";
						sources.model.append (new SearchSource (folder, recursive));
					}
				}
				break;
			}
		}
	}

	public override string to_xml () {
		var doc = new Dom.Document ();
		var root = new Dom.Element.with_attributes ("search", "version", SEARCH_FORMAT);
		doc.append_child (root);
		save_catalog_to_doc (root);
		var sources_node = new Dom.Element ("sources");
		root.append_child (sources_node);
		foreach (unowned var source in sources) {
			sources_node.append_child (source.create_element (doc));
		}
		root.append_child (test.create_element (doc));
		return doc.to_xml ();
	}

	const string SEARCH_FORMAT = "1.0";
}

public class Gth.SearchSource : Object {
	public File folder;
	public bool recursive;

	public SearchSource (File _folder, bool _recursive) {
		folder = _folder;
		recursive = _recursive;
	}

	public Dom.Element create_element (Dom.Document doc) {
		var node = new Dom.Element ("source");
		node.set_attribute ("uri", folder.get_uri ());
		node.set_attribute ("recursive", recursive ? "true" : "false");
		return node;
	}
}
