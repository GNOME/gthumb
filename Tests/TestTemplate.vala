static int n_tests = 0;
static int n_errors = 0;

int main (string[] args) {
	test_tokenize ("", {});
	test_tokenize ("xxx##yy#", { "xxx", "##", "yy", "#" });
	test_tokenize ("日", { "日" });
	test_tokenize ("日本", { "日本" });
	test_tokenize ("#", { "#" });
	test_tokenize ("##", { "##" });
	test_tokenize ("#日", { "#", "日" });
	test_tokenize ("日#", { "日", "#" });
	test_tokenize ("日#本", { "日", "#", "本" });
	test_tokenize ("日#本#", { "日", "#", "本", "#" });
	test_tokenize ("日#本#語", { "日", "#", "本", "#", "語" });
	test_tokenize ("%日", { "%日" });
	test_tokenize ("%日%本", { "%日", "%本" });
	test_tokenize ("%日-%本", { "%日", "-", "%本" });
	test_tokenize ("%日-%本-", { "%日", "-", "%本", "-" });
	test_tokenize ("-%日-%本", { "-", "%日", "-", "%本" });
	test_tokenize ("%日{ 本語 }", { "%日{ 本語 }" });
	test_tokenize ("%日{ 本語 }日本語", { "%日{ 本語 }", "日本語" });
	test_tokenize ("%日{ 本語 }%本", { "%日{ 本語 }", "%本" });
	test_tokenize ("%日{ 本語 }%正{ 體字 }", { "%日{ 本語 }", "%正{ 體字 }" });
	test_tokenize ("%日{ 本語 }%正{ 體字 }{ 本語 }", { "%日{ 本語 }", "%正{ 體字 }{ 本語 }" });
	test_tokenize ("%日{ 本語 }-%正{ 體字 }{ 本語 }", { "%日{ 本語 }", "-", "%正{ 體字 }{ 本語 }" });
	test_tokenize ("%日{ 本語 }-%日{ 本 }{ 語 }-", { "%日{ 本語 }", "-", "%日{ 本 }{ 語 }", "-" });
	test_tokenize ("%Q{ %A{ Prompt: } }", { "%Q{ %A{ Prompt: } }" });
	test_tokenize ("%Q{ { %A } }", { "%Q{ { %A } }" });
	test_tokenize ("%Q{ {} }", { "%Q{ {} }" });
	test_tokenize ("%Q{ { }", { "%Q{ { }" });
	test_tokenize ("%Q{ %A{} }", { "%Q{ %A{} }" });
	test_tokenize ("%Q{ %A{}{} }", { "%Q{ %A{}{} }" });
	test_tokenize ("%Q{ %A{ Prompt: }{ 12 } }", { "%Q{ %A{ Prompt: }{ 12 } }" });
	test_tokenize ("%a{ %x }%b{ %y }", { "%a{ %x }", "%b{ %y }" });
	test_tokenize ("%a{ %x }%b{ %y }%c{ %z }", { "%a{ %x }", "%b{ %y }", "%c{ %z }" });
	test_tokenize ("%a{%x}%b{%y}%c{%z}", { "%a{%x}", "%b{%y}", "%c{%z}" });
	test_tokenize ("%a{%x} %b{%y} %c{%z}", { "%a{%x}", " ", "%b{%y}", " ", "%c{%z}" });

	test_get_token_code ("", 0);
	test_get_token_code ("a", 0);
	test_get_token_code ("日", 0);
	test_get_token_code ("%a", 'a');
	test_get_token_code ("% a", 0);
	test_get_token_code ("%日", '日');
	test_get_token_code ("%日 %字", '日');
	test_get_token_code ("% 日", 0);
	test_get_token_code (" %日", 0);
	test_get_token_code (" % 日", 0);
	test_get_token_code ("%{}", 0);
	test_get_token_code ("%{ text }", 0);
	test_get_token_code ("%{ %字 }", 0);
	test_get_token_code ("%日{}", '日');
	test_get_token_code ("%日{%字}", '日');
	test_get_token_code ("%日{ }", '日');
	test_get_token_code ("%日{ %字 }", '日');

	test_get_token_args ("", {});
	test_get_token_args ("%F", {});
	test_get_token_args ("{ xxx }{ yyy }", {});
	test_get_token_args ("%A%F{ xxx }{ yyy }", {});
	test_get_token_args ("%F{ xxx }{ yyy }", { "xxx", "yyy" });
	test_get_token_args ("%日{ 本語 }", { "本語" });
	test_get_token_args ("%日{ 本語 }{ 體字 }", { "本語", "體字" });
	test_get_token_args ("%Q{ %A }", { "%A" });
	test_get_token_args ("%Q{ %A{ Prompt: } }", { "%A{ Prompt: }" });
	test_get_token_args ("%Q{ %A{ Prompt: }{ 10 } }", { "%A{ Prompt: }{ 10 }" });
	test_get_token_args ("%Q{ %A{ Prompt: }{ 10 } %A{ Prompt2: }{ 20 } }", {
		"%A{ Prompt: }{ 10 } %A{ Prompt2: }{ 20 }"
	});

	test_replace_enumerator ("", 1, null);
	test_replace_enumerator ("#", 1, "1");
	test_replace_enumerator ("#", 10, "10");
	test_replace_enumerator ("##", 1, "01");
	test_replace_enumerator ("##", 99, "99");
	test_replace_enumerator ("##", 100, "100");
	test_replace_enumerator ("####", 1, "0001");
	test_replace_enumerator ("####", 10, "0010");
	test_replace_enumerator ("####", 100, "0100");
	test_replace_enumerator ("####", 1000, "1000");

	print ("\n");
	print ("tests: %d\n", n_tests);
	print ("errors: %d\n", n_errors);
	return 0;
}

void test_replace_enumerator (string token, int value, string? expected) {
	var result = Gth.Template.replace_enumerator (token, value);
	if (result != expected) {
		stderr.printf ("> replace_enumerator ('%s') expecting: '%s'  got: '%s'\n",
				token,
				expected,
				result);
		n_errors++;
	}
	n_tests++;
}

void test_get_token_args (string token, string[] args) {
	var result = Gth.Template.get_token_args (token);
	var idx = 0;
	//stdout.printf ("> '%s'\n", token);
	foreach (unowned var str in result) {
		//stdout.printf ("  '%s'\n", str);
		if (str != args[idx]) {
			stderr.printf ("> get_token_args ('%s') position %d, expecting: '%s'  got: '%s'\n",
				token,
				idx,
				args[idx],
				str);
			n_errors++;
		}
		idx++;
	}
	if (result.length != args.length) {
		stderr.printf ("> get_token_args ('%s') wrong length, expecting: %d  got: %d\n",
				token,
				args.length,
				result.length);
	}
	n_tests++;
}

void test_get_token_code (string token, unichar expected) {
	var result = Gth.Template.get_token_code (token);
	if (result != expected) {
		stderr.printf ("> get_token_code ('%s') expecting: '%s'  got: '%s'\n",
				token,
				expected.to_string (),
				result.to_string ());
		n_errors++;
	}
	n_tests++;
}

void test_tokenize (string template, string[] tokens) {
	var result = Gth.Template.tokenize (template);
	var idx = 0;
	//stdout.printf ("> '%s'\n", template);
	foreach (unowned var str in result) {
		//stdout.printf ("  '%s'\n", str);
		if (str != tokens[idx]) {
			stderr.printf ("> tokenize ('%s') position %d, expecting: '%s'  got: '%s'\n",
				template,
				idx,
				tokens[idx],
				str);
			n_errors++;
		}
		idx++;
	}
	if (result.length != tokens.length) {
		stderr.printf ("> tokenize ('%s') wrong length, expecting: %d  got: %d\n",
				template,
				tokens.length,
				result.length);
	}
	n_tests++;
}
