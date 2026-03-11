public class Gth.Catalog : Object {
	public File? file;
	public string name;
	public string sort_type;
	public bool inverse_order;
	public Gth.Date date;
	public GenericArray<File> files;
	public GenericSet<File> file_set;

	public Catalog () {
		file = null;
		files = new GenericArray<File>();
		file_set = new GenericSet<File> (Util.file_hash, Util.file_equal);
		sort_type = null;
		inverse_order = false;
		date = Gth.Date ();
		name = null;
	}

	public static FileData get_root () {
		var file = File.new_for_uri ("catalog:///");
		var info = new FileInfo ();
		Catalog.update_file_info_for_library (file, info);
		return new FileData (file, info);
	}

	public static bool is_base_dir (File file) {
		return file.get_uri () == "catalog:///";
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
		else {
			catalog = new Catalog ();
			catalog.load_old_format (data);
		}
		if (catalog == null) {
			throw new IOError.FAILED ("Could not load the catalog.");
		}
		catalog.file = file;
		if (catalog.name == null) {
			catalog.name = Catalog.get_name_from_file (catalog.file);
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

	public static File? from_display_name (string name, string extension) {
		try {
			var dir = File.new_for_uri ("catalog:///");
			return dir.get_child_for_display_name (name + extension);
		}
		catch (Error error) {
			return null;
		}
	}

	public static File? from_gdatetime (GLib.DateTime date_time, string extension) {
		try {
			var year = date_time.format ("%Y");
			var dir = File.new_for_uri ("catalog:///" + year +  "/");
			var display_name = date_time.format ("%Y-%m-%d");
			return dir.get_child_for_display_name (display_name + extension);
		}
		catch (Error error) {
			return null;
		}
	}

	public static File get_base_dir () {
		return UserDir.get_directory (FileIntent.READ, DirType.DATA, APP_DIR, "catalogs");
	}

	public static File? make_base_dir () {
		return UserDir.get_directory (FileIntent.WRITE, DirType.DATA, APP_DIR, "catalogs");
	}

	public static void update_file_info_for_library (File file, FileInfo info) {
		info.set_file_type (FileType.DIRECTORY);
		info.set_content_type ("gthumb/library");
		info.set_symbolic_icon (new ThemedIcon ("gth-library-symbolic"));
		info.set_sort_order (0);
		var basename = file.get_basename ();
		if ((basename == null) || (basename == "/")) {
			basename = _("Catalogs");
			info.set_attribute_boolean (FileAttribute.ACCESS_CAN_RENAME, false);
			info.set_attribute_boolean (FileAttribute.ACCESS_CAN_DELETE, false);
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
		info.set_symbolic_icon (new ThemedIcon ("gth-cross-large-symbolic"));
		if (is_search) {
			info.set_content_type ("gthumb/search");
			//info.set_symbolic_icon (new ThemedIcon ("gth-search-symbolic"));
		}
		else {
			info.set_content_type ("gthumb/catalog");
			//info.set_symbolic_icon (new ThemedIcon ("gth-catalog-symbolic"));
		}
		info.set_sort_order (1);
		info.set_attribute_boolean ("gthumb::no-child", true);
		try {
			basename = Util.remove_extension (basename);
			basename = Filename.to_utf8 (basename, -1, null, null);
		}
		catch (Error error) {
		}
		info.set_display_name (basename);
		info.set_edit_name (basename);
	}

	public static string? get_name_from_file (File file) {
		var basename = file.get_basename ();
		if (basename == null) {
			return null;
		}
		try {
			basename = Util.remove_extension (basename);
			basename = Filename.to_utf8 (basename, -1, null, null);
		}
		catch (Error error) {
		}
		return basename;
	}

	public static async Catalog load_from_file (File file, Cancellable cancellable) throws Error {
		var gio_file = Catalog.to_gio_file (file);
		var data = yield Files.load_contents_async (gio_file, cancellable);
		return Catalog.new_from_data (file, data);
	}

	public static async void add_files (File destination, GenericList<File> files, Job job) throws Error {
		var catalog = yield Catalog.load_from_file (destination, job.cancellable);
		var changed = false;
		foreach (unowned var file in files) {
			if (catalog.add_file (file)) {
				changed = true;
			}
		}
		if (changed) {
			yield catalog.save_async (job.cancellable);
			app.events.files_added (catalog.file, files);
		}
	}

	public static async void remove_files (File location, GenericList<File> files, Job job) throws Error {
		var catalog = yield Catalog.load_from_file (location, job.cancellable);
		var changed = false;
		foreach (unowned var file in files) {
			if (catalog.remove_file (file)) {
				changed = true;
			}
		}
		if (changed) {
			yield catalog.save_async (job.cancellable);
			app.events.files_removed_from_catalog (catalog.file, files);
		}
	}

	public static async void rename_files (File destination, GenericList<RenamedFile> renamed_files, Job job) throws Error {
		var catalog = yield Catalog.load_from_file (destination, job.cancellable);
		var changed = false;
		foreach (unowned var renamed in renamed_files) {
			if (catalog.rename_file (renamed.old_file, renamed.new_file)) {
				changed = true;
			}
		}
		if (changed) {
			yield catalog.save_async (job.cancellable);
		}
	}

	public static async void save_order (File location, GenericList<File> files, Job job) throws Error {
		var catalog = yield Catalog.load_from_file (location, job.cancellable);
		catalog.sort_type = "Private::Unsorted";
		catalog.inverse_order = false;
		catalog.remove_all_files ();
		foreach (unowned var file in files) {
			catalog.add_file (file);
		}
		yield catalog.save_async (job.cancellable);
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
				if (child.has_attribute ("type")) {
					sort_type = app.migration.sorter.get_new_key (child.get_attribute ("type"));
					inverse_order = child.get_attribute ("inverse") == "1";
				}
				break;

			case "date":
				date = Date.from_exif_date (child.get_inner_text ());
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
		if (date.is_valid ()) {
			root.append_child (new Dom.Element.with_text ("date", date.to_exif_date ()));
		}

		// Order
		root.append_child (new Dom.Element.with_attributes ("order",
			"type", sort_type,
			"inverse", inverse_order ? "1" : "0"));

		// Files
		var files_node = new Dom.Element ("files");
		root.append_child (files_node);
		foreach (unowned var file in files) {
			files_node.append_child (new Dom.Element.with_attributes ("file",
				"uri", file.get_uri ()));
		}
	}

	public Gth.Catalog? duplicate () {
		try {
			return Catalog.new_from_data (file, to_xml ());
		}
		catch (Error error) {
			return null;
		}
	}

	public virtual void update_file_info (FileInfo info) {
		info.set_file_type (FileType.DIRECTORY);
		info.set_content_type ("gthumb/catalog");
		info.set_symbolic_icon (new ThemedIcon ("gth-catalog-symbolic"));
		info.set_sort_order (1);
		info.set_attribute_boolean ("gthumb::no-child", true);

		// Display name
		var display_name = new StringBuilder ();
		if (name != null) {
			display_name.append (name);
		}
		if (date.is_valid ()) {
			if (display_name.len > 0) {
				display_name.append (" ");
			}
			display_name.append ("(");
			display_name.append (date.to_display_string ());
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
			try {
				var edit_name = Filename.to_utf8 (basename, -1, null, null);
				info.set_edit_name (edit_name);
			}
			catch (Error error) {
				info.set_edit_name (basename);
			}
		}

		// Sort order
		if (sort_type != null) {
			info.set_attribute_string ("sort::type", sort_type);
			info.set_attribute_boolean ("sort::inverse", inverse_order);
		}
		else {
			info.set_attribute_string ("sort::type", "Private::Unsorted");
			info.set_attribute_boolean ("sort::inverse", false);
		}

		// Secondary sort order (date)
		var sort_order = date.to_sort_order ();
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
			try {
				var previous_gio_file = Catalog.to_gio_file (previous_file);
				yield previous_gio_file.delete_async (Priority.DEFAULT, cancellable);
			}
			catch (Error error) {
				if (!(error is IOError.NOT_FOUND)) {
					throw error;
				}
			}
		}
		app.events.catalog_saved (this, previous_file);
	}

	public void remove_all_files () {
		files.length = 0;
		file_set.remove_all ();
	}

	public bool add_file (File file) {
		if (file_set.contains (file)) {
			return false;
		}
		files.add (file);
		file_set.add (file);
		return true;
	}

	public bool remove_file (File file) {
		if (!file_set.contains (file)) {
			return false;
		}
		uint pos;
		if (files.find_with_equal_func (file, Util.file_equal, out pos)) {
			files.remove_index (pos);
		}
		file_set.remove (file);
		return true;
	}

	public bool rename_file (File old_file, File new_file) {
		var changed = false;
		int new_pos = -1;

		// Remove the old file
		if (file_set.contains (old_file)) {
			uint pos;
			if (files.find_with_equal_func (old_file, Util.file_equal, out pos)) {
				files.remove_index (pos);
				file_set.remove (old_file);
				new_pos = (int) pos;
				changed = true;
			}
		}

		// Add the new file at the same position
		if (!file_set.contains (new_file)) {
			if (new_pos >= 0) {
				files.insert (new_pos, new_file);
			}
			else {
				files.add (new_file);
			}
			file_set.add (new_file);
			changed = true;
		}

		return changed;
	}

	void load_old_format (string text) throws Error {
		remove_all_files ();
		var mem_stream = new MemoryInputStream.from_data (text.data, null);
		var data_stream = new DataInputStream (mem_stream);
		var is_search = text.has_prefix ("# Search");
		var list_start = is_search ? 10 : 1;
		var row = 0;
		string line;
		size_t line_length;
		while ((line = data_stream.read_line (out line_length)) != null) {
			row++;
			if ((row > list_start) && (line_length > 2)) {
				var uri = line.substring (1, (long) line_length - 2);
				var file = File.new_for_uri (uri);
				add_file (file);
			}
		}
	}

	const string CATALOG_FORMAT = "1.0";
}
