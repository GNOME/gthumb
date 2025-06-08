public class Gth.Catalog {
	public string name;
	public string sort_type;
	public bool inverse_order;
	public Gth.DateTime date;
	public GenericArray<File> files;

	public Catalog () {
		files = new GenericArray<File>();
		sort_type = null;
		inverse_order = false;
		date = new Gth.DateTime ();
		name = null;
	}

	public static Catalog? new_from_data (string data) throws Error {
		Catalog catalog = null;
		if (data.has_prefix ("<?xml")) {
			var doc = new Dom.Document ();
			doc.load_xml (data);
			if (doc.first_child != null) {
				if (doc.first_child.tag_name == "search") {
					catalog = new CatalogSearch ();
					catalog.load_doc (doc);
				}
				else {
					catalog = new Catalog ();
					catalog.load_doc (doc);
				}
			}
		}
		return catalog;
	}

	public static File? to_gio_file (File file) {
		try {
			var uri_str = file.get_uri ();
			var uri = Uri.parse (uri_str, UriFlags.NONE);
			unowned var query = uri.get_query ();
			if (query != null) {
				var child_uri = Uri.unescape_string (query, null);
				return File.new_for_uri (child_uri);
			}
			else {
				var base_dir = Catalog.get_base_dir ();
				unowned var path = uri.get_path ();
				var full_uri = base_dir.get_uri () + "/" + path;
				return File.new_for_uri (full_uri);
			}
		}
		catch (Error error) {
			return null;
		}
	}

	public static File get_base_dir () {
		return UserDir.get_directory (FileIntent.READ, DirType.DATA, APP_DIR, "catalogs");
	}

	public static void update_file_info_for_library (File file, FileInfo info) {
		info.set_file_type (FileType.DIRECTORY);
		info.set_content_type ("gthumb/library");
		info.set_symbolic_icon (new ThemedIcon ("library-symbolic"));
		info.set_sort_order (0);
		if (file.get_uri () == "catalog:///") {
			info.set_display_name (_("Catalogs"));
			info.set_edit_name (_("Catalogs"));
		}
	}

	public void load_doc (Dom.Document doc) {
		foreach (unowned var child in doc.first_child) {
			switch (child.tag_name) {
			case "files":
				foreach (unowned var node in child) {
					unowned var uri = node.get_attribute ("uri");
					if (uri != null) {
						files.add (File.new_for_uri (uri));
					}
				}
				break;

			case "order":
				sort_type = child.get_attribute ("type");
				inverse_order = child.get_attribute ("inverse") == "1";
				break;

			case "date":
				date.set_from_exif_date (child.get_inner_text ());
				break;

			case "name":
				name = child.get_inner_text ();
				stdout.printf ("child.tag_name: %s\n", name);
				break;
			}
		}
	}

	public virtual void update_file_info (File file, FileInfo info) {
		info.set_file_type (FileType.DIRECTORY);
		info.set_content_type ("gthumb/catalog");
		info.set_symbolic_icon (new ThemedIcon ("catalog-symbolic"));
		info.set_sort_order (1);
		info.set_attribute_boolean ("gthumb::no-child", true);

		var display_name = new StringBuilder ();
		if (name != null) {
			display_name.append (name);
		}
		if (date.date.is_valid ()) {
			if (display_name.len > 0) {
				display_name.append (" ");
			}
			display_name.append ("(");
			display_name.append (date.date.to_display_string ());
			display_name.append (")");
		}
		if ((display_name.len == 0) && (file != null)) {
			try {
				var basename = file.get_basename ();
				var name = Filename.to_utf8 (basename, -1, null, null);
				display_name.append (Util.remove_extension (name));
			}
			catch (Error error) {
			}
		}
		info.set_display_name (display_name.str);
		info.set_edit_name (display_name.str);

		var sort_order = date.date.to_sort_order ();
		if (sort_order != 0) {
			info.set_attribute_uint32 ("gth::standard::secondary-sort-order", sort_order);
		}
		else {
			info.remove_attribute ("gth::standard::secondary-sort-order");
		}
	}
}
