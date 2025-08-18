public class Gth.Scripts {
	public GenericList<Script> entries;

	public Scripts () {
		entries = new GenericList<Script> ();
		loaded = false;
	}

	public async void load_from_file () {
		if (loaded) {
			return;
		}
		entries.model.remove_all ();
		var local_job = app.new_job ("Load scripts");
		try {
			var scripts_file = UserDir.get_config_file (FileIntent.READ, SCRIPTS_FILE);
			var bytes = yield Files.load_file_async (scripts_file, local_job.cancellable);
			unowned var contents = (string) bytes.get_data ();
			var doc = new Dom.Document ();
			doc.load_xml (contents);
			if ((doc.first_child != null) && (doc.first_child.tag_name == "scripts")) {
				foreach (unowned var node in doc.first_child) {
					if (node.tag_name == "script") {
						var script = new Gth.Script ();
						script.load_from_element (node);
						entries.model.append (script);
					}
				}
			}
		}
		catch (Error error) {
			// Ignored
		}
		local_job.done ();
		loaded = true;
		//app.monitor.scripts_changed ();
	}

	bool loaded;
}
