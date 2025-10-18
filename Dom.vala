public class Dom.Element : InitiallyUnowned {
	public string tag_name;
	public HashTable<string, string> attributes;
	public Element next_sibling;
	public weak Element previous_sibling;
	public weak Element parent_node;
	public Element first_child;
	public weak Element last_child;

	public Element (string _tag_name) {
		tag_name = _tag_name;
		attributes = new HashTable<string, string> (str_hash, str_equal);
		next_sibling = null;
		previous_sibling = null;
		parent_node = null;
		first_child = null;
		last_child = null;
	}

	public Element.with_attributes (string _tag_name, ...) {
		this (_tag_name);
		var list = va_list ();
		while (true) {
			unowned string? id = list.arg ();
			if (id == null)
				break;
			unowned string? value = list.arg ();
			if (value == null)
				break;
			set_attribute (id, value);
		}
	}

	public Element.with_text (string _tag_name, string? text_data) {
		this (_tag_name);
		append_child (new TextNode (text_data));
	}

	public void append_child (Element child) {
		child.parent_node = this;
		child.next_sibling = null;
		if (last_child != null) {
			last_child.next_sibling = child;
			child.previous_sibling = last_child;
		}
		else {
			child.previous_sibling = null;
			first_child = child;
		}
		last_child = child;
	}

#if TEST_XML
	public void remove_children () {
		next_sibling = null;
		previous_sibling = null;
		parent_node = null;
		first_child = null;
		last_child = null;
	}
#endif

	public unowned string get_attribute (string name) {
		return attributes.lookup (name);
	}

	public bool get_attribute_as_int (string name, out int result) {
		var value = get_attribute (name);
		if (value == null) {
			result = 0;
			return false;
		}
		return int.try_parse (value, out result, null, 10);
	}

	public bool get_attribute_as_int64 (string name, out int64 result) {
		var value = get_attribute (name);
		if (value == null) {
			result = 0;
			return false;
		}
		return int64.try_parse (value, out result, null, 10);
	}

	public bool get_attribute_as_double (string name, out double result) {
		var value = get_attribute (name);
		if (value == null) {
			result = 0;
			return false;
		}
		return double.try_parse (value, out result, null);
	}

	public bool get_attribute_as_float (string name, out float result) {
		var value = get_attribute (name);
		if (value == null) {
			result = 0;
			return false;
		}
		return float.try_parse (value, out result, null);
	}

	public bool has_attribute (string name) {
		return attributes.contains (name);
	}

	public void set_attribute (string name, string? value) {
		if (value != null) {
			attributes.insert (name, value);
		}
		else {
			attributes.remove (name);
		}
	}

	public virtual unowned string? get_inner_text () {
		if (first_child is TextNode)
			return first_child.get_inner_text ();
		return null;
	}

	const int TAB_WIDTH = 2;

	public static void append_attribute (StringBuilder str, string value) {
		var escaped = Markup.escape_text (value);
		Gth.Strings.escape_xml_quote (str, escaped);
	}

	protected const string NEW_LINE = "\n";

	public virtual void append_xml (StringBuilder xml, int level) {
		if ((parent_node != null) && (this != parent_node.first_child)) {
			xml.append (NEW_LINE);
		}

		var indentation = (level > 0) ? string.nfill (level * TAB_WIDTH, ' ') : "";
		var include_tag = !tag_name.has_prefix ("#");
		if (include_tag) {
			xml.append (indentation);
			xml.append_c ('<');
			xml.append (tag_name);

			// Attributes
			var keys = attributes.get_keys ();

#if TEST_XML
			// Sorted by name to make the xml testable
			keys.sort (string.collate);
#endif

			foreach (unowned var key in keys) {
				unowned var value = attributes[key];
				xml.append_c (' ');
				xml.append (key);
				xml.append_c ('=');
				xml.append_c ('"');
				Dom.Element.append_attribute (xml, value);
				xml.append_c ('"');
			};

			if (first_child == null) {
				xml.append ("/>");
				return;
			}

			xml.append_c ('>');
			if ((first_child != null) && !(first_child is TextNode)) {
				xml.append (NEW_LINE);
			}
		}

		foreach (unowned var node in this) {
			node.append_xml (xml, level + 1);
		}

		if (include_tag) {
			if ((first_child != null) && !(first_child is TextNode)) {
				xml.append (NEW_LINE);
				xml.append (indentation);
			}
			xml.append ("</");
			xml.append (tag_name);
			xml.append_c ('>');
		}
	}

	// Children iterator
	public Iterator iterator () {
		return new Iterator (this);
	}

	public class Iterator {
		private Element node;
		private weak Element? child;
		private bool started;

		public Iterator (Element _node) {
			node = _node;
			child = null;
			started = false;
		}

		public bool next () {
			if (!started) {
				child = node.first_child;
				started = true;
			}
			else {
				child = child.next_sibling;
			}
			return child != null;
		}

		public unowned Element get () {
			return child;
		}
	}
}

public class Dom.TextNode : Dom.Element {
	public string data;

	public TextNode (string? _data) {
		base ("#text");
		data = _data;
	}

	public override unowned string? get_inner_text () {
		return data;
	}

	public override void append_xml (StringBuilder str, int level) {
		if (data != null) {
			str.append (Markup.escape_text (data));
		}
	}
}

public class Dom.Document : Dom.Element {
	public Document () {
		base ("#document");
	}

	public void load_file (File file) throws Error {
		var contents = Gth.Files.load_contents (file);
		load_xml (contents);
	}

	public void load_xml (string buffer) throws Error {
		open_nodes = new Queue<Element> ();
		open_nodes.push_head (this);
		var context = new MarkupParseContext (Parser, 0, this, null);
		context.parse (buffer, buffer.length);
		context.end_parse ();
	}

	public string to_xml () {
		var xml = new StringBuilder ();
		xml.append ("""<?xml version="1.0" encoding="UTF-8"?>""");
		xml.append (NEW_LINE);
		append_xml (xml, -1);
		if (first_child != null)
			xml.append (NEW_LINE);
		return xml.str;
	}

	private void visit_start (MarkupParseContext context, string name, string[] attr_names, string[] attr_values) throws MarkupError {
		var head = open_nodes.peek_head ();
		if (head == null)
			throw new MarkupError.INVALID_CONTENT ("No open node");

		var node = new Dom.Element (name);
		for (int i = 0; attr_names[i] != null; i++) {
			node.set_attribute (attr_names[i], attr_values[i]);
		}
#if TEST_XML
		// Do not add empty text node when testing.
		if (head.last_child is TextNode)
			head.remove_children ();
#endif
		head.append_child (node);
		open_nodes.push_head (node);
	}

	private void visit_end (MarkupParseContext context, string name) throws MarkupError {
		open_nodes.pop_head ();
	}

	private void visit_text (MarkupParseContext context, string text, size_t text_len) throws MarkupError {
		if (text_len == 0)
			return;
		var head = open_nodes.peek_head ();
		if (head == null)
			throw new MarkupError.INVALID_CONTENT ("No open node");
#if TEST_XML
		// Do not add empty text node when testing.
		if (head.last_child != null)
			return;
#endif
		var node = new Dom.TextNode (text);
		head.append_child (node);
	}

	private const MarkupParser Parser = {
		visit_start,
		visit_end,
		visit_text,
		null, // passthrough
		null, // error
	};

	Queue<Element> open_nodes;
}
