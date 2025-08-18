public class Gth.Script : Object {
	public class Code {
		public const unichar URI = 'U';
		public const unichar PATH = 'F';
		public const unichar BASENAME = 'B';
		public const unichar BASENAME_NO_EXTENSION = 'N';
		public const unichar EXTENSION = 'E';
		public const unichar PARENT_PATH = 'P';
		public const unichar TIMESTAMP = 'T';
		public const unichar ASK_VALUE = '?';
		public const unichar FILE_ATTRIBUTE = 'A';
		public const unichar QUOTE = 'Q';
	}

	public string id {
		get { return _id; }
		set {
			_id = value;
			_detailed_action = Action.print_detailed_name ("exec-script", new Variant.string (_id));
		}
	}

	public string detailed_action { get { return _detailed_action; } }
	public string display_name { get; set; }
	public string command { get; set; }
	public bool visible { get; set; }
	public bool shell_script { get; set; }
	public bool for_each_file { get; set; }
	public bool wait_command { get; set; }
	public string accelerator { get; set; }

	public Script () {
		Object (id: Strings.new_random (ID_LENGTH));
	}

	public string? get_requested_attributes () {
		var result = new StringBuilder ();
		Template.for_each_token (command, TemplateFlags.NO_ENUMERATOR, (parent_code, code, args) => {
			if (code == Code.FILE_ATTRIBUTE) {
				if (result.len > 0) {
					result.append_c (',');
				}
				result.append (args[0]);
			}
			return false;
		});
		return (result.len > 0) ? result.str : null;
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

	string _id;
	string _detailed_action;

	const int ID_LENGTH = 8;
}
