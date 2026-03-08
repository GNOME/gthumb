public class Gth.Script : Object {
	public string id {
		get { return _id; }
		set {
			_id = value;
			_detailed_action = Action.print_detailed_name ("exec-script", new Variant.string (_id));
		}
	}

	public string detailed_action { get { return _detailed_action; } }
	public string display_name { get; set; default = ""; }
	public string command { get; set; default = ""; }
	public bool visible { get; set; default = false; }
	public bool shell_script { get; set; default = false; }
	public bool for_each_file { get; set; default = false; }
	public bool wait_command { get; set; default = false; }
	public string accelerator { get; set; default = null; }

	public Script () {
		Object (id: Strings.new_random (ID_LENGTH));
	}

	public Dom.Element create_element (Dom.Document doc) {
		var node = new Dom.Element ("script");
		node.set_attribute ("id", id);
		node.set_attribute ("display-name", display_name);
		node.set_attribute ("command", command);
		node.set_attribute ("shell-script", (shell_script ? "true" : "false"));
		node.set_attribute ("for-each-file", (for_each_file ? "true" : "false"));
		node.set_attribute ("wait-command", (wait_command ? "true" : "false"));
		if (!visible) {
			node.set_attribute ("display", "none");
		}
		return node;
	}

	public void load_from_element (Dom.Element node) {
		id = node.get_attribute ("id");
		display_name = node.get_attribute ("display-name");
		command = node.get_attribute ("command");
		visible = node.get_attribute ("display") != "none";
		shell_script = node.get_attribute ("shell-script") == "true";
		for_each_file = node.get_attribute ("for-each-file") == "true";
		wait_command = node.get_attribute ("wait-command") == "true";
		accelerator = "";
	}

	public Script duplicate () {
		var new_script = new Script ();
		var doc = new Dom.Document ();
		var node = this.create_element (doc);
		doc.append_child (node);
		new_script.load_from_element (node);
		return new_script;
	}

	public void copy (Script other) {
		var doc = new Dom.Document ();
		var node = other.create_element (doc);
		doc.append_child (node);
		load_from_element (node);
	}

	public Gth.Shortcut create_shortcut () {
		var shortcut = new Shortcut ("win.exec-script", new Variant.string (id));
		shortcut.description = display_name;
		shortcut.context = SHORTCUT_CONTEXT_BROWSER_VIEWER;
		shortcut.category = ShortcutCategory.SCRIPTS;
		//shortcut.set_accelerator ();
		return shortcut;
	}

	public async void execute (Gth.MainWindow window, GenericList<File> files) {
		var local_job = window.new_job (display_name, JobFlags.FOREGROUND, "gth-script-symbolic");
		try {
			var attributes = get_requested_attributes ();
			var file_data_list = yield FileManager.query_list_info (files, attributes, QueryListFlags.NOT_RECURSIVE, local_job.cancellable);
			if (for_each_file) {
				var total = file_data_list.model.n_items;
				var iter = file_data_list.iterator ();
				while (iter.next ()) {
					var file = iter.get ();
					local_job.progress = Util.calc_progress (iter.index (), total);
					local_job.subtitle = file.get_display_name ();
					var list = new GenericList<FileData>();
					list.model.append (file);
					try {
						var command_line = yield get_command_line (window, list, iter.has_next (), local_job);
						yield exec_command (command_line, local_job);
					}
					catch (Error error) {
						if (!(error is ScriptError.SKIPPED)) {
							throw error;
						}
					}
				}
			}
			else {
				var command_line = yield get_command_line (window, file_data_list, false, local_job);
				yield exec_command (command_line, local_job);
			}
		}
		catch (Error error) {
			if (!(error is IOError.CANCELLED)) {
				var toast = Util.new_literal_toast (error.message);
				toast.button_label = _("Details");
				toast.action_name = "win.view-script-output";
				toast.priority = Adw.ToastPriority.HIGH;
				toast.timeout = 0;
				window.script_output = process.read_output ();
				window.add_toast (toast);
			}
		}
		finally {
			local_job.done ();
		}
	}

	public string get_preview (TemplateFlags flags = TemplateFlags.DEFAULT) {
		var script_template = new ScriptTemplate (command, flags | TemplateFlags.NO_ENUMERATOR | TemplateFlags.PREVIEW);
		var preview = script_template.eval ();
		//stdout.printf ("> script command preview: %s\n", preview);
		return preview;
	}

	ScriptProcess process = null;

	async void exec_command (string command, Job job) throws Error {
		//stdout.printf ("> exec_command: %s\n", command);

		string[] argv;
		if (shell_script) {
			argv = { "sh", "-c", command, null };
		}
		else {
			Shell.parse_argv (command, out argv);
		}

		process = new ScriptProcess ();
		process.wait_command = wait_command;
		process.spawn (argv);
		if (wait_command) {
			yield process.wait_process (job.cancellable);
			// var output = process.read_output ();
			// if (output != null) {
			// 	stdout.printf ("> OUTPUT: %s\n", (string) output.get_data ());
			// }
		}
	}

	async string get_command_line (Gth.MainWindow window, GenericList<FileData> files, bool can_skip, Job job) throws Error {
		var template = new ScriptTemplate (command, TemplateFlags.NO_ENUMERATOR);
		template.files = files;
		return yield template.read_parameters (window, display_name, can_skip, job);
	}

	string get_requested_attributes () {
		var result = new StringBuilder ();
		result.append (STANDARD_ATTRIBUTES);
		Template.for_each_token (command, TemplateFlags.NO_ENUMERATOR, (parent_code, code, args) => {
			if (code == ScriptTemplate.Code.FILE_ATTRIBUTE) {
				result.append_c (',');
				result.append (args[0]);
			}
			return false;
		});
		return result.str;
	}

	string _id;
	string _detailed_action;

	const int ID_LENGTH = 8;
}


errordomain Gth.ScriptError {
	SKIPPED
}


class Gth.ScriptProcess {
	public bool wait_command;

	public ScriptProcess () {
		wait_command = false;
	}

	public void spawn (string[] argv) throws Error {
		output_memory = new MemoryOutputStream (null, GLib.realloc, GLib.free);
		output_stream = new DataOutputStream (output_memory);

		// Add the command to execute to the output stream.
		output_stream.put_string ("$");
		foreach (unowned var arg in argv) {
			if (arg != null) {
				output_stream.put_string (" ");
				output_stream.put_string (arg);
			}
		}
		output_stream.put_string ("\n\n");

		var flags = SpawnFlags.SEARCH_PATH;
		if (wait_command) {
			flags |= SpawnFlags.DO_NOT_REAP_CHILD;
		}
		string[] env = Environ.get ();
		int standard_input;
		int standard_output;
		int standard_error;
		Process.spawn_async_with_pipes (null, argv, env, flags, child_setup,
			out child_pid,
			out standard_input,
			out standard_output,
			out standard_error);
		output = new IOChannel.unix_new (standard_output);
		error = new IOChannel.unix_new (standard_error);
	}

	public async void wait_process (Cancellable cancellable) throws Error {
		cancellable.cancelled.connect (() => {
			if (child_pid != 0) {
				Posix.killpg (child_pid, Posix.Signal.TERM);
			}
		});
		wait_callback = wait_process.callback;
		wait_error = null;
		watch_id = ChildWatch.add (child_pid, (pid, status) => {
			Process.close_pid (pid);
			child_pid = 0;
			watch_id = 0;
			if (status != 0) {
				wait_error = new IOError.FAILED (_("Command exited abnormally with status %d").printf (status));
			}
			wait_callback ();
		});
		yield;
		if (wait_error != null) {
			throw wait_error;
		}
	}

	public Bytes? read_output () {
		read_channel (output);
		read_channel (error);
		try {
			output_stream.close ();
		}
		catch (Error error) {
			// ignored.
		}
		try {
			output_memory.close ();
			return output_memory.steal_as_bytes ();
		}
		catch (Error error) {
			return null;
		}
	}

	void read_channel (IOChannel channel) {
		if (channel != null) {
			try {
				string line;
				size_t length = -1;
				size_t terminator_pos = -1;
				while (channel.read_line (out line, out length, out terminator_pos) != IOStatus.EOF) {
					if (length > 0) {
						output_stream.write (line.data[0:terminator_pos]);
						output_stream.put_string ("\n");
					}
				}
			}
			catch (Error error) {
			}
		}
	}

	void child_setup () {
		// Detach from the TTY
		Posix.setsid ();

		if (wait_command) {
			// Create a process group to kill all the child processes when
			// cancelling the operation.
			Posix.setpgid (0, 0);
		}
	}

	Pid child_pid;
	SourceFunc wait_callback;
	Error wait_error;
	uint watch_id;
	IOChannel output;
	IOChannel error;
	MemoryOutputStream output_memory;
	DataOutputStream output_stream;
}
