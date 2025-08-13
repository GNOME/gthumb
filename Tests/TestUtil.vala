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

	test_get_digits (0, 1);
	test_get_digits (1, 1);
	test_get_digits (9, 1);
	test_get_digits (10, 2);
	test_get_digits (11, 2);
	test_get_digits (99, 2);
	test_get_digits (100, 3);
	test_get_digits (101, 3);
	test_get_digits (12345, 5);
	test_get_digits (-1, 2);
	test_get_digits (-9, 2);
	test_get_digits (-10, 3);
	test_get_digits (-11, 3);
	test_get_digits (-99, 3);
	test_get_digits (-50, 3);
	test_get_digits (-100, 4);
	test_get_digits (-101, 4);

	print ("\n");
	print ("tests: %d\n", n_tests);
	print ("errors: %d\n", n_errors);
	return 0;
}

void test_get_digits (int number, int expected) {
	var result = Gth.Util.get_digits (number);
	if (result != expected) {
		stderr.printf ("> get_digits (%d)  expecting: %d  got: %d\n", number, expected, result);
		n_errors++;
	}
	n_tests++;
}

void test_remove_extension (string filename, string expected) {
	var result = Gth.Util.remove_extension (filename);
	if (result != expected) {
		stderr.printf ("> remove_extension ('%s')  expecting: '%s'  got: '%s'\n", filename, expected, result);
		n_errors++;
	}
	n_tests++;
}
