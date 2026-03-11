static int n_tests = 0;
static int n_errors = 0;

int main (string[] args) {
	test_ends_with ("abc.tar.xz", "", -1, -1, true);
	test_ends_with ("abc.tar.xz", "z", -1, -1, true);
	test_ends_with ("abc.tar.xz", ".xz", -1, -1, true);
	test_ends_with ("abc.tar.xz", ".tar", 4, 7, true);
	test_get_number_in_name ("abc", null);
	test_get_number_in_name ("abc123", "123");
	test_get_number_in_name ("abc123xyz789", "123");
	test_get_number_in_name ("日123本789語", "123");

	print ("\n");
	print ("tests: %d\n", n_tests);
	print ("errors: %d\n", n_errors);
	return 0;
}

void test_ends_with (string str, string trail, int trail_offset, int str_offset, bool expected) {
	var result = Gth.Strings.ends_with (str, trail, trail_offset, str_offset);
	if (result != expected) {
		stderr.printf ("> ends_with ('%s', '%s', %d, %d)  expecting: %s  got: %s\n",
			str, trail, trail_offset, str_offset,
			expected.to_string (),
			result.to_string ());
		n_errors++;
	}
	n_tests++;
}

void test_get_number_in_name (string name, string? expected) {
	var result = Gth.Strings.get_number_in_name (name);
	if (result != expected) {
		stderr.printf ("> get_number_in_name ('%s')  expecting: %s  got: %s\n",
			name,
			expected,
			result);
		n_errors++;
	}
	n_tests++;
}
