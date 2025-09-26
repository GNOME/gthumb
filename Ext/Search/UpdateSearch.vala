public class Gth.UpdateSearch {
	public UpdateSearch () {
		search = null;
	}

	public async void update_file (Browser browser, File file, Job job) throws Error {
		var gio_file = Catalog.to_gio_file (file);
		var data = yield Files.load_contents_async (gio_file, job.cancellable);
		var catalog = Catalog.new_from_data (file, data);
		if (!(catalog is CatalogSearch)) {
			throw new IOError.FAILED ("Invalid catalog");
		}
		search = catalog as CatalogSearch;
		yield search_and_save (browser, search, file, job);
	}

	async void search_and_save (Browser browser, CatalogSearch _search, File _file, Job job) throws Error {
		// Search.

		Error error = null;
		try {
			yield update_search (browser, _search, _file, job);
		}
		catch (Error _error) {
			error = _error;
		}

		// Save the search result.

		var local_cancellable = new Cancellable ();
		yield search.save_async (local_cancellable);
		app.monitor.file_created (search.file);

		// End of search.

		if (toast != null) {
			toast.dismiss ();
		}
		if ((error != null) && !(error is IOError.CANCELLED)) {
			throw error;
		}
		browser.update_folder_status ();
	}

	async void update_search (Browser browser, CatalogSearch _search, File _file, Job job) throws Error {
		search = _search;
		search.file = _file;
		toast = null;

		// Clear the search results.

		search.clear_files ();
		yield search.save_async (job.cancellable);
		app.monitor.file_created (search.file);

		// Open the catalog.

		var load_action = browser.folder_tree.current_folder.file.equal (search.file) ? LoadAction.OPEN_SUBFOLDER : LoadAction.OPEN;
		yield browser.open_location_async (search.file, load_action, job);

		// Show a message on the browser.

		browser.folder_status.title = _("Searching Files");

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
			var flags = ForEachFlags.DEFAULT;
			if (folder.recursive) {
				flags |= ForEachFlags.RECURSIVE;
			}
			var list_attr = browser.get_list_attributes ();
			var all_attr = Util.concat_attributes (list_attr, test.attributes);
			yield source.foreach_child (folder.folder, flags, all_attr, job.cancellable, (child, is_parent) => {
				var action = ForEachAction.CONTINUE;
				if (is_parent) {
					if (child.info.get_is_hidden () && !include_hidden) {
						action = ForEachAction.SKIP;
					}
					else {
						job.subtitle = child.get_display_name ();
					}
				}
				else {
					if (test.match (child)) {
						search.add_file (child.file);
						browser.add_to_search_results (search.file, child);
					}
				}
				return action;
			});
			if (job.cancellable.is_cancelled ()) {
				break;
			}
		}
	}

	CatalogSearch search;
	Adw.Toast toast;
}
