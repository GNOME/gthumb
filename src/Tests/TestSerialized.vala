static int n_tests = 0;
static int n_errors = 0;

int main (string[] args) {
	var str = new Gth.Serialized.string ("hello");
	check_deserialized (str);

	var arr = new Gth.Serialized.array ();
	check_deserialized (arr);

	arr = new Gth.Serialized.array ();
	arr.add_string ("hello");
	check_deserialized (arr);

	arr = new Gth.Serialized.array ();
	arr.add_string ("hello");
	arr.add_string ("world");
	check_deserialized (arr);

	var obj = new Gth.Serialized.object ();
	check_deserialized (obj);

	obj = new Gth.Serialized.object ();
	obj.set_string ("timestamp", "2023-02-19T14:43:24.940124Z");
	check_deserialized (obj);

	obj = new Gth.Serialized.object ();
	obj.set_string ("timestamp1", "2023-02-19T14:43:24.940124Z");
	obj.set_string ("timestamp2", "2026-05-22T15:53:05.781292Z");
	check_deserialized (obj);

	obj = new Gth.Serialized.object ();
	obj.set ("arr", arr);
	obj.set ("str", str);
	check_deserialized (obj);

	print ("\n");
	print ("tests: %d\n", n_tests);
	print ("errors: %d\n", n_errors);
	return 0;
}

void check_deserialized (Gth.Serialized serialized) {
	var deserialized = new Gth.Serialized.from_bytes (serialized.to_bytes ());
	if (!deserialized.equal (serialized)) {
		stderr.printf ("Expected:\n");
		stderr.printf ("%s", serialized.to_debug ());
		stderr.printf ("Got:\n");
		stderr.printf ("%s\n", deserialized.to_debug ());
		n_errors++;
	}
	n_tests++;
}
