public class Gth.Test : Object {
	public enum Operation {
		NONE,
		EQUAL,
		LOWER,
		GREATER,
		CONTAINS,
		CONTAINS_ALL,
		CONTAINS_ONLY,
		STARTS_WITH,
		ENDS_WITH,
		MATCHES,
		BEFORE,
		AFTER;

		public string to_xml_attribute () {
			return XML_ATTRIBUTE[this];
		}

		public static Operation from_xml_attribute (string attr) {
			var idx = -1;
			for (var i = 0; i < XML_ATTRIBUTE.length; i++) {
				if (attr == XML_ATTRIBUTE[i]) {
					idx = i;
				}
			}
			if ((idx >= EQUAL) && (idx <= AFTER)) {
				return (Operation) idx;
			}
			return NONE;
		}

		const string[] XML_ATTRIBUTE = {
			"none",
			"equal",
			"lower",
			"greater",
			"contains",
			"contains-all",
			"contains-only",
			"starts-with",
			"ends-with",
			"matches",
			"before",
			"after",
		};
	}

	public struct OperationInfo {
		string name;
		Test.Operation op;
		bool negative;
	}

	public static const Test.OperationInfo[] Int_Operations = {
		{ N_("Lower than"), Test.Operation.LOWER, false },
		{ N_("Greater than"), Test.Operation.GREATER, false },
		{ N_("Equal to"), Test.Operation.EQUAL, false },
		{ N_("Greater than or Equal to"), Test.Operation.LOWER, true },
		{ N_("Lower than or Equal to"), Test.Operation.GREATER, true }
	};

	public struct SizeInfo {
		string display_name;
		int64 size;
	}

	public const SizeInfo[] Unit_List = {
		{ N_("kB"), 1024 },
		{ N_("MB"), 1024*1024 },
		{ N_("GB"), 1024*1024*1024 },
	};

	public string id { get; set; default = ""; }
	public string display_name { get; set; default = ""; }
	public string attributes { get; set; default = ""; }
	public bool visible { get; set; default = false; }
	public GenericArray<FileData> files = null;

	public signal void changed ();

	public virtual bool match (FileData file) {
		return false;
	}

	public virtual Gtk.Widget? create_options () {
		return null;
	}

	public virtual void update_from_options () throws Error {
		// void
	}

	public virtual void focus_options () {
		// void
	}

	public virtual void set_files (GenericArray<FileData> _files) {
		files = _files;
		iterator = 0;
	}

	public FileData? get_next () {
		if (files != null) {
			while (iterator < files.length) {
				unowned var file = files[iterator++];
				if (match (file)) {
					return file;
				}
			}
		}
		return null;
	}

	public virtual Dom.Element create_element (Dom.Document doc) {
		var node = new Dom.Element ("test");
		node.set_attribute("id", id);
		if (!visible) {
			node.set_attribute ("display", "none");
		}
		return node;
	}

	public virtual void load_from_element (Dom.Element node) {
		id = node.get_attribute ("id");
		visible = node.get_attribute ("display") != "none";
	}

	public Gth.Test duplicate () {
		var new_test = Object.new (this.get_class ().get_type ()) as Gth.Test;
		var doc = new Dom.Document ();
		var node = this.create_element (doc);
		doc.append_child (node);
		new_test.load_from_element (node);
		return new_test;
	}

	int iterator = 0;
}
