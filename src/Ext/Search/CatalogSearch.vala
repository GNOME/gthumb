public class Gth.CatalogSearch : Gth.Catalog {
	public GenericList<SearchSource> sources;
	public TestExpr? test;

	public CatalogSearch () {
		sources = new GenericList<SearchSource>();
		test = null;
	}

	public override void update_file_info (FileInfo info) {
		base.update_file_info (info);
		info.set_content_type ("gthumb/search");
		info.set_symbolic_icon (new ThemedIcon ("gth-search-symbolic"));
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
		return to_xml_generic ();
	}

	public bool equal_search_parameters (CatalogSearch other) {
		var xml_1 = to_xml_generic (true);
		var xml_2 = other.to_xml_generic (true);
		//stdout.printf ("> XML(1) : %s\n", xml_1);
		//stdout.printf ("> XML(2) : %s\n", xml_2);
		//stdout.printf ("> EQUAL: %s\n", (xml_1 == xml_2).to_string ());
		return xml_1 == xml_2;
	}

	string to_xml_generic (bool only_search_parameters = false) {
		var doc = new Dom.Document ();
		var root = new Dom.Element.with_attributes ("search", "version", SEARCH_FORMAT);
		doc.append_child (root);
		if (!only_search_parameters) {
			save_catalog_to_doc (root);
		}
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
