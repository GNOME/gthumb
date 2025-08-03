public class Gth.Catalog : Object {
	public File? file;
	public string name;
	public string sort_type;
	public bool inverse_order;
	public Gth.DateTime date;
	public GenericArray<File> files;
	public GenericSet<File> file_set;

	public Catalog () {
		file = null;
		files = new GenericArray<File>();
		file_set = new GenericSet<File> (Util.file_hash, Util.file_equal);
		sort_type = null;
		inverse_order = false;
		date = new Gth.DateTime ();
		name = null;
	}

	public static Catalog? new_from_data (File file, string data) throws Error {
		Catalog catalog = null;
		if (data.has_prefix ("<?xml")) {
			var doc = new Dom.Document ();
			doc.load_xml (data);
			if (doc.first_child != null) {
				if (doc.first_child.tag_name == "search") {
					catalog = new CatalogSearch ();
				}
				else {
					catalog = new Catalog ();
				}
				catalog.load_doc (doc);
			}
		}
		if (catalog != null) {
			catalog.file = file;
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

	public static File? from_gio_file (File file) {
		var base_dir = Catalog.get_base_dir ();
		var base_uri = base_dir.get_uri ();
		var file_uri = file.get_uri ();
		if (!file_uri.has_prefix (base_uri)) {
			return null;
		}
		var catalog_uri = "catalog://" + file_uri.substring (base_uri.length);
		//stdout.printf ("  '%s' => '%s'\n", file_uri, catalog_uri);
		return File.new_for_uri (catalog_uri);
	}

	public static File get_base_dir () {
		return UserDir.get_directory (FileIntent.READ, DirType.DATA, APP_DIR, "catalogs");
	}

	public static void update_file_info_for_library (File file, FileInfo info) {
		info.set_file_type (FileType.DIRECTORY);
		info.set_content_type ("gthumb/library");
		info.set_symbolic_icon (new ThemedIcon ("library-symbolic"));
		info.set_sort_order (0);
		var basename = file.get_basename ();
		if ((basename == null) || (basename == "/")) {
			basename = _("Catalogs");
		}
		info.set_display_name (basename);
		info.set_edit_name (basename);
	}

	public static void update_file_info_for_broken_file (File file, FileInfo info) {
		var basename = file.get_basename ();
		if ((basename == null) || (basename == "/")) {
			Catalog.update_file_info_for_library (file, info);
			return;
		}
		info.set_file_type (FileType.DIRECTORY);
		var is_search = basename.has_suffix (".search");
		info.set_symbolic_icon (new ThemedIcon ("cross-large-symbolic"));
		if (is_search) {
			info.set_content_type ("gthumb/search");
			//info.set_symbolic_icon (new ThemedIcon ("search-symbolic"));
		}
		else {
			info.set_content_type ("gthumb/catalog");
			//info.set_symbolic_icon (new ThemedIcon ("catalog-symbolic"));
		}
		info.set_sort_order (1);
		info.set_attribute_boolean ("gthumb::no-child", true);
		basename = Filename.to_utf8 (basename, -1, null, null);
		basename = Util.remove_extension (basename);
		info.set_display_name (basename);
		info.set_edit_name (basename);
	}

	public virtual void load_doc (Dom.Document doc) {
		foreach (unowned var child in doc.first_child) {
			switch (child.tag_name) {
			case "files":
				foreach (unowned var node in child) {
					unowned var uri = node.get_attribute ("uri");
					if (uri != null) {
						add_file (File.new_for_uri (uri));
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
				break;
			}
		}
	}

	public virtual string to_xml () {
		var doc = new Dom.Document ();
		var root = new Dom.Element.with_attributes ("catalog", "version", CATALOG_FORMAT);
		doc.append_child (root);
		save_catalog_to_doc (root);
		return doc.to_xml ();
	}

	public void save_catalog_to_doc (Dom.Element root) {
		// Name
		if (name != null) {
			root.append_child (new Dom.Element.with_text ("name", name));
		}

		// Date
		if (date.date_is_valid ()) {
			root.append_child (new Dom.Element.with_text ("date", date.to_exif_date ()));
		}

		// Files
		var files_node = new Dom.Element ("files");
		root.append_child (files_node);
		foreach (unowned var file in files) {
			files_node.append_child (new Dom.Element.with_attributes ("file", "uri", file.get_uri ()));
		}
	}

	public Gth.Catalog duplicate () {
		return Catalog.new_from_data (file, to_xml ());
	}

	public virtual void update_file_info (FileInfo info) {
		info.set_file_type (FileType.DIRECTORY);
		info.set_content_type ("gthumb/catalog");
		info.set_symbolic_icon (new ThemedIcon ("catalog-symbolic"));
		info.set_sort_order (1);
		info.set_attribute_boolean ("gthumb::no-child", true);

		// Display name
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

		// Edit name
		if (name != null) {
			info.set_edit_name (name);
		}
		else if (file != null) {
			var basename = file.get_basename ();
			var edit_name = Filename.to_utf8 (basename, -1, null, null);
			info.set_edit_name (edit_name);
		}

		// Sort order
		if (sort_type != null) {
			info.set_attribute_string ("sort::type", sort_type);
			info.set_attribute_boolean ("sort::inverse", inverse_order);
		}
		else {
			info.remove_attribute ("sort::type");
			info.remove_attribute ("sort::inverse");
		}

		// Secondary sort order (date)
		var sort_order = date.date.to_sort_order ();
		if (sort_order != 0) {
			info.set_attribute_uint32 ("Private::SecondarySortOrder", sort_order);
		}
		else {
			info.remove_attribute ("Private::SecondarySortOrder");
		}
	}

	public async void save_async (Cancellable cancellable) throws Error {
		var previous_file = file;

		// Update file
		var extension = Util.get_extension (file.get_basename ());
		var filename = Util.clear_for_filename (name) + "." + extension;
		var parent = file.get_parent ();
		file = parent.get_child_for_display_name (filename);

		// Save
		var gio_file = Catalog.to_gio_file (file);
		var buffer = to_xml ();
		var bytes = new Bytes.static (buffer.data);
		yield Files.save_file_async (gio_file, bytes, cancellable);

		// Delete the previous file if different
		if (!file.equal (previous_file)) {
			var previous_gio_file = Catalog.to_gio_file (previous_file);
			yield previous_gio_file.delete_async (Priority.DEFAULT, cancellable);
			app.monitor.file_renamed (previous_file, file);
		}
	}

	public void clear_files () {
		files.length = 0;
	}

	public void add_file (File file) {
		if (file_set.contains (file)) {
			return;
		}
		files.add (file);
		file_set.add (file);
	}

	const string CATALOG_FORMAT = "1.0";
}
