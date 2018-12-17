#include <args/parser.hpp>
#include <string_view>
#include <iostream>

struct test {
	const char* title;
	int(*callback)();
	int expected{ 0 };
};

std::vector<test> g_tests;

template <typename Test>
struct registrar {
	registrar() {
		::g_tests.push_back({ Test::get_name(), Test::run, Test::expected()}); \
	}
};

#define TEST_BASE(name, EXPECTED) \
	struct test_ ## name { \
		static const char* get_name() noexcept { return #name; } \
		static int run(); \
		static int expected() noexcept { return (EXPECTED); } \
	}; \
	registrar<test_ ## name> reg_ ## name; \
	int test_ ## name ::run()

#define TEST(name) TEST_BASE(name, 0)
#define TEST_FAIL(name) TEST_BASE(name, 1)

int main(int argc, char* argv[]) {
	if (argc == 1) {
		for (auto const& test : g_tests) {
			printf("%d:%s\n", test.expected, test.title);
		}
		return 0;
	}

	auto const int_test = atoi(argv[1]);
	if (int_test < 0)
		return 100;
	auto const test = static_cast<size_t>(int_test);
	return g_tests[test].callback();
}

template <typename ... CString, typename Mod>
int every_test_ever(Mod mod, CString ... args) {
	std::string arg_opt;
	std::string arg_req;
	bool starts_as_false{ false };
	bool starts_as_true{ true };
	std::vector<std::string> multi_opt;
	std::vector<std::string> multi_req;
	std::string positional;

	char arg0[] = "args-help-test";
	char* __args[] = { arg0, (const_cast<char*>(args))..., nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;
	::args::null_translator tr;
	::args::parser p{ "program description", argc, __args, &tr };
	p.arg(arg_opt, "o", "opt").meta("VAR").help("a help for arg_opt").opt();
	p.arg(arg_req, "r", "req").help("a help for arg_req");
	p.set<std::true_type>(starts_as_false, "on", "1").help("a help for on").opt();
	p.set<std::false_type>(starts_as_true, "off", "0").help("a help for off").opt();
	p.arg(multi_opt, "first").help("zero or more").opt();
	p.arg(multi_req, "second").meta("VAL").help("one or more");
	p.arg(positional).meta("INPUT").help("a help for positional").opt();
	mod(p);
	p.parse();
	return 0;
}

void noop(const args::parser&) {}
void modify(args::parser& parser) {
	parser.program("another");
	if (parser.program() != "another") {
		fprintf(stderr, "Program not changed: %s\n", parser.program().c_str());
		std::exit(1);
	}
	parser.usage("[OPTIONS]");
	if (parser.usage() != "[OPTIONS]") {
		fprintf(stderr, "Usage not changed: %s\n", parser.usage().c_str());
		std::exit(1);
	}
}

template <typename T, typename U>
void EQ_impl(T&& lhs, U&& rhs, const char* lhs_name, const char* rhs_name) {
	if (lhs == rhs)
		return;
	std::cerr << "Expected equality of these values:\n "
		<< lhs_name << "\n    Which is: " << lhs << "\n "
		<< rhs_name << "\n    Which is: " << rhs << "\n";
	std::exit(1);
}

#define EQ(lhs, rhs) EQ_impl(lhs, rhs, #lhs, #rhs)

TEST(gen_usage) {
	return every_test_ever([](args::parser& parser) {
		std::string shrt;
		const std::string_view expected_help = "args-help-test [-h] [-o VAR] -r ARG [--on] [--off] [--first ARG ...] --second VAL [--second VAL ...] [INPUT]";
		parser.printer_append_usage(shrt);
		EQ(expected_help, shrt);
	}, "-r", "x", "--second", "somsink");
}

TEST(gen_usage_no_help) {
	return every_test_ever([](args::parser& parser) {
		std::string shrt;
		const std::string_view expected_no_help = "args-help-test [-o VAR] -r ARG [--on] [--off] [--first ARG ...] --second VAL [--second VAL ...] [INPUT]";
		parser.provide_help(false);
		parser.printer_append_usage(shrt);
		EQ(expected_no_help, shrt);
	}, "-r", "x", "--second", "somsink");
}

TEST(short_help_argument) {
	return every_test_ever(noop, "-h");
}

TEST(long_help_argument) {
	return every_test_ever(noop, "--help");
}

TEST(help_mod) {
	return every_test_ever(modify, "-h");
}

TEST_FAIL(no_req) {
	return every_test_ever(noop);
}

TEST_FAIL(no_req_mod) {
	return every_test_ever(modify);
}

// TODO: used to be test_fail; regeresion or code behind fixed?
TEST(full) {
	return every_test_ever(noop,
		"-oVALUE",
		"-r", "SEPARATE",
		"--req", "ANOTHER ONE",
		"--on",
		"-10",
		"--off",
		"--second", "somsink",
		"POSITIONAL");
}

TEST_FAIL(missing_arg_short) {
	return every_test_ever(noop, "-r");
}

TEST_FAIL(missing_arg) {
	return every_test_ever(noop, "--req");
}

TEST_FAIL(missing_positional) {
	std::vector<std::string> one_plus;
	return every_test_ever([&](args::parser& p) {
		p.arg(one_plus)
			.meta("ARG")
			.help("this parameter must be given at least once");
	}, "-r", "x", "--second", "somsink");
}

TEST_FAIL(unknown) {
	return every_test_ever(noop, "--flag");
}

TEST_FAIL(unknown_short) {
	return every_test_ever(noop, "-f");
}

TEST_FAIL(unknown_positional) {
	char arg0[] = "args-help-test";
	char arg1[] = "POSITIONAL";
	char* __args[] = { arg0, arg1, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;
	::args::null_translator tr;
	::args::parser p{ "program description", argc, __args, &tr };
	p.parse();
	return 0;

}

TEST(console_width) {
	const auto isatty = args::detail::is_terminal(stdout);
	const auto width = args::detail::terminal_width(stdout);
	return isatty ? (width ? 0 : 1) : (width ? 1 : 0);
}