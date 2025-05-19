public Gth.Application app;

int main (string[] args) {
	Intl.bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	Intl.bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	Intl.textdomain (GETTEXT_PACKAGE);

	app = new Gth.Application ();
	var status = app.run (args);
	if (app.restart) {
		try {
			var cmd = new StringBuilder (args[0]);
			Process.spawn_command_line_async (cmd.str);
		}
		catch (SpawnError e) {
			error ("%s", e.message);
		}
	}
	return status;
}
