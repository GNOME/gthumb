public Gth.Application app;

int main (string[] args) {
	app = new Gth.Application ();
	var status = app.run (args);
	if (app.restart) {
		try {
			var cmd = new StringBuilder (args[0]);
			//if (app.profile_id != "")
			//	cmd.append_printf (" --profile=%s", app.profile_id);
			Process.spawn_command_line_async (cmd.str);
		}
		catch (SpawnError e) {
			error ("%s", e.message);
		}
	}
	return status;
}
