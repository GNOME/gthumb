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
		process.spawn (argv, wait_command);
		if (wait_command) {
			yield process.wait_process (job);
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
	public void spawn (string[] argv, bool _wait_command) throws Error {
		wait_command = _wait_command;
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
		output.set_encoding (null);
		output.set_buffered (false);
		output.add_watch (IOCondition.IN | IOCondition.PRI | IOCondition.HUP, (channel, condition) => {
			if (condition == IOCondition.HUP) {
				return false;
			}
			read_channel (channel);
			return true;
		});

		error = new IOChannel.unix_new (standard_error);
		error.set_encoding (null);
		error.set_buffered (false);
		error.add_watch (IOCondition.IN | IOCondition.PRI | IOCondition.HUP, (channel, condition) => {
			if (condition == IOCondition.HUP) {
				return false;
			}
			read_channel (channel);
			return true;
		});
	}

	public async void wait_process (Job job) throws Error {
		cancellable = job.cancellable;
		cancellable.cancelled.connect (() => {
			if (child_pid != 0) {
				wait_error = new IOError.CANCELLED ("Cancelled");
				Posix.killpg (child_pid, Posix.Signal.TERM);
				if (wait_timeout == 0) {
					wait_timeout = Util.after_seconds (MAX_WAIT, () => stop_waiting ());
				}
			}
		});
		wait_callback = wait_process.callback;
		wait_error = null;
		watch_id = ChildWatch.add (child_pid, (pid, status) => on_process_exit (pid, status));
		yield;
		if (wait_error != null) {
			throw wait_error;
		}
	}

	void on_process_exit (Pid pid, int status) {
		watch_id = 0;
		if (child_pid == 0) {
			return;
		}
		if (wait_timeout != 0) {
			Source.remove (wait_timeout);
			wait_timeout = 0;
		}
		Process.close_pid (child_pid);
		child_pid = 0;
		if ((status != 0) && (wait_error == null)) {
			wait_error = new IOError.FAILED (_("Command exited abnormally with status %d").printf (status));
		}
		wait_callback ();
	}

	void stop_waiting () {
		wait_timeout = 0;
		if (child_pid == 0) {
			return;
		}
		if (watch_id != 0) {
			Source.remove (watch_id);
			watch_id = 0;
		}
		Process.close_pid (child_pid);
		child_pid = 0;
		wait_callback ();
	}

	public Bytes? read_output () {
		try {
			read_channel_to_end (output);
			read_channel_to_end (error);
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

	char buffer[2];

	bool read_channel (IOChannel channel) {
		if (channel == null) {
			return false;
		}
		try {
			size_t bytes;
			channel.read_chars (buffer, out bytes);
			if (bytes == 0) {
				return false;
			}
			output_stream.write ((uint8[]) buffer[0:bytes]);
			return true;
		}
		catch (Error error) {
			return false;
		}
	}

	inline void read_channel_to_end (IOChannel channel) {
		while (read_channel (channel)) { /* void */ }
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

	bool wait_command;
	Pid child_pid;
	SourceFunc wait_callback;
	Error wait_error;
	uint wait_timeout = 0;
	uint watch_id;
	IOChannel output;
	IOChannel error;
	MemoryOutputStream output_memory;
	DataOutputStream output_stream;
	Cancellable cancellable;

	const uint MAX_WAIT = 5; // seconds
}
