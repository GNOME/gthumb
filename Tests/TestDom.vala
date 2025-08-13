static int n_tests = 0;
static int n_errors = 0;

int main (string[] args) {
	// Empty
	var doc = new Dom.Document ();
	test_doc (doc,	"""<?xml version="1.0" encoding="UTF-8"?>
""");

	// Empty <filters>
	doc = new Dom.Document ();
	var filters = new Dom.Element.with_attributes ("filters", "version", "1.0");
	doc.append_child (filters);
	test_doc (doc,	"""<?xml version="1.0" encoding="UTF-8"?>
<filters version="1.0"/>
""");

	// Filter
	doc = new Dom.Document ();
	filters = new Dom.Element.with_attributes ("filters", "version", "1.0");
	doc.append_child (filters);

	var filter = new Dom.Element.with_attributes ("filter", "name", "test1");
	filters.append_child (filter);

	var match = new Dom.Element.with_attributes ("match", "type", "all");
	filter.append_child (match);

	var test = new Dom.Element ("test");
	test.set_attribute ("id", "::filesize");
	test.set_attribute ("op", "lt");
	test.set_attribute ("value", "10");
	test.set_attribute ("unit", "kB");
	match.append_child (test);

	test = new Dom.Element ("test");
	test.set_attribute ("id", "::filename");
	test.set_attribute ("op", "contains");
	test.set_attribute ("value", "logo");
	match.append_child (test);

	var limit = new Dom.Element ("limit");
	limit.set_attribute ("value", "25");
	limit.set_attribute ("type", "files");
	limit.set_attribute ("selected_by", "more_recent");
	filter.append_child (limit);

	test_doc (doc,	"""<?xml version="1.0" encoding="UTF-8"?>
<filters version="1.0">
  <filter name="test1">
    <match type="all">
      <test id="::filesize" op="lt" unit="kB" value="10"/>
      <test id="::filename" op="contains" value="logo"/>
    </match>
    <limit selected_by="more_recent" type="files" value="25"/>
  </filter>
</filters>
""");

	print ("\n");
	print ("tests: %d\n", n_tests);
	print ("errors: %d\n", n_errors);
	return 0;
}

void test_doc (Dom.Document doc, string expected) {
	check_loaded_xml (doc);
	check_xml (doc,	expected);
}

void check_xml (Dom.Document doc, string expected) {
	var xml = doc.to_xml ();
	if (xml != expected) {
		stderr.printf ("Expected:\n");
		stderr.printf ("%s", expected);
		stderr.printf ("\nGot:\n");
		stderr.printf ("%s", xml);
		n_errors++;
	}
	n_tests++;
}

void check_loaded_xml (Dom.Document doc) {
	try {
		var xml = doc.to_xml ();
		var loaded_doc = new Dom.Document ();
		loaded_doc.load_xml (xml);
		var loaded_xml = loaded_doc.to_xml ();
		if (xml != loaded_xml) {
			stderr.printf ("Expected:\n");
			stderr.printf ("%s", xml);
			stderr.printf ("\nGot:\n");
			stderr.printf ("%s", loaded_xml);
			n_errors++;
		}
	}
	catch (Error error) {
		stderr.printf ("%s", error.message);
		n_errors++;
	}
	n_tests++;
}
