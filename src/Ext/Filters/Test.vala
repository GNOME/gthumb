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

	public const Test.OperationInfo[] Int_Operations = {
		{ N_("Greater Than"), Test.Operation.GREATER, false },
		{ N_("Greater Than or Equal To"), Test.Operation.LOWER, true },
		{ N_("Lower Than"), Test.Operation.LOWER, false },
		{ N_("Lower Than or Equal To"), Test.Operation.GREATER, true },
		{ N_("Equal To"), Test.Operation.EQUAL, false },
	};

	public struct SizeInfo {
		string display_name;
		int64 size;
	}

	public const SizeInfo[] Unit_List = {
		// Translators: short for kilobytes
		{ N_("kB"), 1024 },
		// Translators: short for megabytes
		{ N_("MB"), 1024 * 1024 },
		// Translators: short for gigabytes
		{ N_("GB"), 1024 * 1024 * 1024 },
	};

	public string id { get; set; default = ""; }
	public string display_name { get; set; default = ""; }
	public string title { get; set; default = null; }
	public string attributes { get; set; default = ""; }
	public bool visible { get; set; default = false; }
	public GenericArray<FileData> files = null;

	public signal void options_changed ();

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

	public virtual TestIterator iterator (GenericList<FileData> files) {
		return new TestIterator (this, files);
	}

	public virtual Dom.Element create_element (Dom.Document doc) {
		var node = new Dom.Element ("test");
		node.set_attribute ("id", app.migration.test.get_old_key (id));
		if (!visible) {
			node.set_attribute ("display", "none");
		}
		return node;
	}

	public virtual void load_from_element (Dom.Element node) {
		id = app.migration.test.get_new_key (node.get_attribute ("id"));
		visible = node.get_attribute ("display") != "none";
	}

	public virtual void copy (Gth.Test other) {
		var doc = new Dom.Document ();
		var node = other.create_element (doc);
		doc.append_child (node);
		load_from_element (node);
	}

	public Gth.Test duplicate () {
		var new_test = Object.new (this.get_class ().get_type ()) as Gth.Test;
		var doc = new Dom.Document ();
		var node = this.create_element (doc);
		doc.append_child (node);
		new_test.load_from_element (node);
		return new_test;
	}

	public bool has_attributes () {
		return !Strings.empty (attributes);
	}

	public string to_string () {
		var doc = new Dom.Document ();
		var node = create_element (doc);
		doc.append_child (node);
		return doc.to_xml ();
	}

	public Gth.Shortcut create_shortcut () {
		var shortcut = new Shortcut ("win.set-filter", new Variant.string (id));
		shortcut.description = display_name;
		shortcut.context = ShortcutContext.BROWSER;
		shortcut.category = ShortcutCategory.FILTERS;
		//shortcut.set_accelerator ();
		return shortcut;
	}
}

public class Gth.TestIterator {
	public TestIterator (Test _test, GenericList<FileData> _files) {
		test = _test;
		files = _files;
		file_index = 0;
		file = null;
	}

	public unowned FileData? get () {
		return file;
	}

	public virtual bool next () {
		while (true) {
			file = files.model.get_item (file_index++) as FileData;
			if ((file == null) || test.match (file)) {
				break;
			}
		}
		return file != null;
	}

	public int index () {
		return file_index;
	}

	protected Test test;
	protected GenericList<FileData> files;
	protected int file_index;
	protected FileData file;
}
