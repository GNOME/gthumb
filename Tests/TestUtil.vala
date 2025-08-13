static int n_tests = 0;
static int n_errors = 0;

int main (string[] args) {
	test_remove_extension ("", "");
	test_remove_extension ("abc", "abc");
	test_remove_extension ("abc.x", "abc");
	test_remove_extension ("abc.xyz", "abc");
	test_remove_extension ("abc.tar", "abc");
	test_remove_extension ("abc.tar.x", "abc");
	test_remove_extension ("abc.tar.xz", "abc");
	test_remove_extension (".hidden", ".hidden");

	print ("\n");
	print ("tests: %d\n", n_tests);
	print ("errors: %d\n", n_errors);
	return 0;
}

void test_remove_extension (string filename, string expected) {
	var result = Gth.Util.remove_extension (filename);
	if (result != expected) {
		stderr.printf ("> remove_extension ('%s')  expecting: '%s'  got: '%s'\n", filename, expected, result);
		n_errors++;
	}
	n_tests++;
}
