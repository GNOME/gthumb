public class Gth.UpdateSearch {
	public UpdateSearch () {
		search = null;
	}

	public async void update_file (Browser browser, File file, Job job) throws Error {
		// Load the catalog.

		var gio_file = Catalog.to_gio_file (file);
		var data = yield Files.load_contents_async (gio_file, job.cancellable);
		var catalog = Catalog.new_from_data (file, data);
		if (!(catalog is CatalogSearch)) {
			throw new IOError.FAILED ("Invalid catalog");
		}
		search = catalog as CatalogSearch;

		// Search.

		Error error = null;
		try {
			// Clear the search results.

			search.remove_all_files ();
			yield search.save_async (job.cancellable);
			app.events.file_created (search.file);

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

			// Search each folder.

			var include_hidden = browser.show_hidden_files;
			var list_attr = browser.get_list_attributes ();
			var all_attr = Util.concat_attributes (list_attr, test.attributes);
			foreach (var folder in search.sources) {
				var flags = ForEachFlags.DEFAULT;
				if (folder.recursive) {
					flags |= ForEachFlags.RECURSIVE;
				}
				var source = app.get_source_for_file (folder.folder);
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
			}
		}
		catch (Error _error) {
			error = _error;
		}

		// Save the catalog.
		var local_cancellable = new Cancellable ();
		yield search.save_async (local_cancellable);
		app.events.file_created (search.file);
		browser.update_folder_status ();

		if ((error != null) && !(error is IOError.CANCELLED)) {
			throw error;
		}
	}

	CatalogSearch search;
}
