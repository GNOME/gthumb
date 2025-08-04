public class Gth.UpdateSearch {
	public UpdateSearch () {
		search = null;
	}

	public async void search_and_save (Browser browser, CatalogSearch _search, File _file, Cancellable cancellable) throws Error {
		// Search.

		Error error = null;
		try {
			yield update_search (browser, _search, _file, cancellable);
		}
		catch (Error _error) {
			error = _error;
		}

		// Save the search result.

		var local_cancellable = new Cancellable ();
		yield search.save_async (local_cancellable);

		// End of search.

		if (toast != null) {
			toast.dismiss ();
		}
		if ((error != null) && !(error is IOError.CANCELLED)) {
			throw error;
		}
	}

	async void update_search (Browser browser, CatalogSearch _search, File _file, Cancellable cancellable) throws Error {
		search = _search;
		search.file = _file;
		toast = null;

		// Clear the search results.

		search.clear_files ();
		yield search.save_async (cancellable);
		app.monitor.directory_created (search.file);

		// Open the search in the browser

		yield browser.open_location_async (search.file);

		// Show a message on the browser.

		toast = new Adw.Toast (_("Searching Files"));
		toast.timeout = 0;
		toast.action_name = "win.stop-search";
		toast.button_label = _("Stop");
		toast.use_markup = false;
		browser.window.add_toast (toast);

		// Get the file test.

		Gth.Test test;
		if (!search.test.contains_type_test ()) {
			var test_with_general_filter = new Gth.TestExpr (TestExpr.Operation.INTERSECTION);
			test_with_general_filter.add (app.get_general_filter ());
			test_with_general_filter.add (search.test);
			test = test_with_general_filter;
		}
		else {
			test = search.test;
		}

		// Search

		var include_hidden = browser.show_hidden_files;
		foreach (var folder in search.sources) {
			var source = app.get_source_for_file (folder.folder);
			var flags = ForEachFlags.FOLLOW_LINKS;
			if (folder.recursive) {
				flags |= ForEachFlags.RECURSIVE;
			}
			var list_attr = browser.get_list_attributes ();
			var all_attr = Util.concat_attributes (list_attr, test.attributes);
			yield source.foreach_child (folder.folder, flags, all_attr, cancellable, (child, is_parent) => {
				var action = ForEachAction.CONTINUE;
				if (is_parent) {
					if (child.info.get_is_hidden () && !include_hidden) {
						action = ForEachAction.SKIP;
					}
				}
				else {
					if (test.match (child)) {
						//stdout.printf ("> %s\n", child.file.get_uri ());
						search.add_file (child.file);
						browser.add_to_search_results (search.file, child);
						app.monitor.file_created (search.file, child.file, browser.window);
					}
				}
				return action;
			});
		}
	}

	public async void update_file (Browser browser, File file, Cancellable cancellable) throws Error {
		var gio_file = Catalog.to_gio_file (file);
		var data = yield Files.load_contents_async (gio_file, cancellable);
		var catalog = Catalog.new_from_data (file, data);
		if (!(catalog is CatalogSearch)) {
			throw new IOError.FAILED ("Not a search");
		}
		search = catalog as CatalogSearch;
		search_and_save (browser, search, file, cancellable);
	}

	CatalogSearch search;
	Adw.Toast toast;
	GenericList.Iterator<SearchSource> source_iterator;
}
