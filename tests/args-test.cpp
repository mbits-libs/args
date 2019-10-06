#include <args/parser.hpp>
#include <string_view>
#include <iostream>

using namespace std::literals;

struct test {
	const char* title;
	int(*callback)();
	int expected{ 0 };
	std::string_view output{ };
};

std::vector<test> g_tests;

template <typename Test>
struct registrar {
	registrar() {
		::g_tests.push_back({ Test::get_name(), Test::run, Test::expected(), Test::output() }); \
	}
};

#define TEST_BASE(name, EXPECTED, OUTPUT) \
	struct test_ ## name { \
		static const char* get_name() noexcept { return #name; } \
		static int run(); \
		static int expected() noexcept { return (EXPECTED); } \
		static std::string_view output() noexcept { return OUTPUT; } \
	}; \
	registrar<test_ ## name> reg_ ## name; \
	int test_ ## name ::run()

#define TEST(name) TEST_BASE(name, 0, {})
#define TEST_FAIL(name) TEST_BASE(name, 1, {})
#define TEST_OUT(name, OUTPUT) TEST_BASE(name, 0, OUTPUT)
#define TEST_FAIL_OUT(name, OUTPUT) TEST_BASE(name, 1, OUTPUT)

int main(int argc, char* argv[]) {
	if (argc == 1) {
		for (auto const& test : g_tests) {
			printf("%d:%s:", test.expected, test.title);
			if (!test.output.empty())
				printf("%.*s", static_cast<int>(test.output.size()), test.output.data());
			putc('\n', stdout);
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
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
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
		<< std::boolalpha
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

TEST_OUT(short_help_argument, R"(usage: args-help-test [-h] [-o VAR] -r ARG [--on] [--off] [--first ARG ...] --second VAL [--second VAL ...] [INPUT]\n\nprogram description\n\npositional arguments:\n INPUT         a help for positional\n\noptional arguments:\n -h, --help    show this help message and exit\n -o, --opt VAR a help for arg_opt\n -r, --req ARG a help for arg_req\n --on, -1      a help for on\n --off, -0     a help for off\n --first ARG   zero or more\n --second VAL  one or more\n)"sv) {
	return every_test_ever(noop, "-h");
}

TEST_OUT(long_help_argument, R"(usage: args-help-test [-h] [-o VAR] -r ARG [--on] [--off] [--first ARG ...] --second VAL [--second VAL ...] [INPUT]\n\nprogram description\n\npositional arguments:\n INPUT         a help for positional\n\noptional arguments:\n -h, --help    show this help message and exit\n -o, --opt VAR a help for arg_opt\n -r, --req ARG a help for arg_req\n --on, -1      a help for on\n --off, -0     a help for off\n --first ARG   zero or more\n --second VAL  one or more\n)"sv) {
	return every_test_ever(noop, "--help");
}

TEST(help_mod) {
	return every_test_ever(modify, "-h");
}

TEST_FAIL_OUT(no_req, R"(usage: args-help-test [-h] [-o VAR] -r ARG [--on] [--off] [--first ARG ...] --second VAL [--second VAL ...] [INPUT]\nargs-help-test: error: argument -r is required\n)"sv) {
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

TEST_FAIL_OUT(missing_arg_short, R"(usage: args-help-test [-h] [-o VAR] -r ARG [--on] [--off] [--first ARG ...] --second VAL [--second VAL ...] [INPUT]\nargs-help-test: error: argument -r: expected one argument\n)"sv) {
	return every_test_ever(noop, "-r");
}

TEST_FAIL_OUT(missing_arg, R"(usage: args-help-test [-h] [-o VAR] -r ARG [--on] [--off] [--first ARG ...] --second VAL [--second VAL ...] [INPUT]\nargs-help-test: error: argument --req: expected one argument\n)"sv) {
	return every_test_ever(noop, "--req");
}

TEST_FAIL_OUT(missing_positional, R"(usage: args-help-test [-h] [-o VAR] -r ARG [--on] [--off] [--first ARG ...] --second VAL [--second VAL ...] [INPUT] POSITIONAL [POSITIONAL ...]\nargs-help-test: error: argument POSITIONAL is required\n)"sv) {
	std::vector<std::string> one_plus;
	return every_test_ever([&](args::parser& p) {
		p.arg(one_plus)
			.meta("POSITIONAL")
			.help("this parameter must be given at least once");
		}, "-r", "x", "--second", "somsink");
}

TEST_FAIL_OUT(unknown, R"(usage: args-help-test [-h] [-o VAR] -r ARG [--on] [--off] [--first ARG ...] --second VAL [--second VAL ...] [INPUT]\nargs-help-test: error: unrecognized argument: --flag\n)"sv) {
	return every_test_ever(noop, "--flag");
}

TEST_FAIL_OUT(unknown_short, R"(usage: args-help-test [-h] [-o VAR] -r ARG [--on] [--off] [--first ARG ...] --second VAL [--second VAL ...] [INPUT]\nargs-help-test: error: unrecognized argument: -f\n)"sv) {
	return every_test_ever(noop, "-f");
}

TEST_FAIL_OUT(unknown_positional, R"(usage: args-help-test [-h]\nargs-help-test: error: unrecognized argument: POSITIONAL\n)"sv) {
	char arg0[] = "/usr/bin/args-help-test";
	char arg1[] = "POSITIONAL";
	char* __args[] = { arg0, arg1, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;
	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.parse();
	return 0;

}

TEST(console_width) {
	const auto isatty = args::detail::is_terminal(stdout);
	const auto width = args::detail::terminal_width(stdout);
	return isatty ? (width ? 0 : 1) : (width ? 1 : 0);
}

TEST_OUT(width_forced, R"(usage: args-help-test [-h] [INPUT]\n\nThis is a very long description of the\nprogram, which should span multiple\nlines in narrow consoles. This will be\ntested with forcing a console width in\nthe parse() method.\n\npositional arguments:\n INPUT      This is a very long\n            description of the INPUT\n            param, which should span\n            multiple lines in narrow\n            consoles. This will be\n            tested with forcing a\n            console width in the\n            parse() method. Also,\n            here's a long word:\n            supercalifragilisticexpiali\n            docious\n\noptional arguments:\n -h, --help show this help message and\n            exit\n)"sv) {
	std::string positional;

	char arg0[] = "args-help-test";
	char arg1[] = "-h";
	char* __args[] = { arg0, arg1, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	auto prog_descr =
		"This is a very long description of the program, "
		"which should span multiple lines in narrow consoles. "
		"This will be tested with forcing a console width in "
		"the parse() method."s;
	auto long_descr =
		"This is a very long description of the INPUT param, "
		"which should span multiple lines in narrow consoles. "
		"This will be tested with forcing a console width in "
		"the parse() method. Also, here's a long word: "
		"supercalifragilisticexpialidocious"s;

	::args::null_translator tr;
	::args::parser p{ std::move(prog_descr), ::args::from_main(argc, __args), &tr };
	p.arg(positional).meta("INPUT").help(std::move(long_descr)).opt();
	p.parse(::args::parser::exclusive_parser, 40);
	return 0;
}

TEST_FAIL(not_an_int) {
	int value;

	char arg0[] = "args-help-test";
	char arg1[] = "--num";
	char arg2[] = "value";
	char* __args[] = { arg0, arg1, arg2, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.arg(value, "num").meta("NUMBER").opt();
	p.parse();

	return 0;
}

TEST_FAIL(out_of_range) {
	int value;

	char arg0[] = "args-help-test";
	char arg1[] = "--num";
	char arg2[] = "123456789012345678901234567890123456789012345678901234567890";
	char* __args[] = { arg0, arg1, arg2, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.arg(value, "num").meta("NUMBER").opt();
	p.parse();

	return 0;
}

TEST(optional_int_1) {
	std::optional<int> value;

	char arg0[] = "args-help-test";
	char arg1[] = "--num";
	char arg2[] = "12345";
	char* __args[] = { arg0, arg1, arg2, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.arg(value, "num").meta("NUMBER");
	p.parse();

	constexpr auto has_value = true;
	constexpr auto the_value = 12345;
	EQ(has_value, !!value);
	EQ(the_value, *value);

	return 0;
}

TEST(optional_int_2) {
	std::optional<int> value;

	char arg0[] = "args-help-test";
	char* __args[] = { arg0, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.arg(value, "num").meta("NUMBER");
	p.parse();

	constexpr auto no_value = false;
	EQ(no_value, !!value);

	return 0;
}

TEST(subcmd_long) {
	char arg0[] = "args-help-test";
	char arg1[] = "--num";
	char arg2[] = "12345";
	char* __args[] = { arg0, arg1, arg2, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.parse(::args::parser::allow_subcommands);

	return 0;
}

TEST(subcmd_short) {
	char arg0[] = "args-help-test";
	char arg1[] = "-n";
	char arg2[] = "12345";
	char* __args[] = { arg0, arg1, arg2, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.parse(::args::parser::allow_subcommands);

	return 0;
}

TEST(subcmd_positional) {
	char arg0[] = "args-help-test";
	char arg1[] = "a_path";
	char arg2[] = "12345";
	char* __args[] = { arg0, arg1, arg2, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.parse(::args::parser::allow_subcommands);

	return 0;
}

TEST(custom_simple_1) {
	char arg0[] = "args-help-test";
	char arg1[] = "--path";
	char* __args[] = { arg0, arg1, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.custom([] {}, "path");
	p.parse();

	return 0;
}

TEST(custom_simple_1_exit) {
	char arg0[] = "args-help-test";
	char arg1[] = "--path";
	char* __args[] = { arg0, arg1, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.custom([] { std::exit(0); }, "path");
	p.parse();

	return 1;
}

TEST(custom_simple_2) {
	char arg0[] = "args-help-test";
	char arg1[] = "--path";
	char* __args[] = { arg0, arg1, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.custom([](::args::parser&) {}, "path");
	p.parse();

	return 0;
}

TEST(custom_simple_2_exit) {
	char arg0[] = "args-help-test";
	char arg1[] = "--path";
	char* __args[] = { arg0, arg1, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.custom([](::args::parser&) { std::exit(0); }, "path");
	p.parse();

	return 1;
}

TEST(custom_string_1) {
	char arg0[] = "args-help-test";
	char arg1[] = "--path";
	char arg2[] = "value";
	char* __args[] = { arg0, arg1, arg2, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.custom([](std::string const&) {}, "path");
	p.parse();

	return 0;
}

TEST(custom_string_1_exit) {
	char arg0[] = "args-help-test";
	char arg1[] = "--path";
	char arg2[] = "value";
	char* __args[] = { arg0, arg1, arg2, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.custom([](std::string const&) { std::exit(0); }, "path");
	p.parse();

	return 1;
}

TEST(custom_string_2) {
	char arg0[] = "args-help-test";
	char arg1[] = "--path";
	char arg2[] = "value";
	char* __args[] = { arg0, arg1, arg2, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.custom([](::args::parser&, std::string const&) {}, "path");
	p.parse();

	return 0;
}

TEST(custom_string_2_exit) {
	char arg0[] = "args-help-test";
	char arg1[] = "--path";
	char arg2[] = "value";
	char* __args[] = { arg0, arg1, arg2, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	::args::null_translator tr;
	::args::parser p{ "program description", ::args::from_main(argc, __args), &tr };
	p.custom([](::args::parser&, std::string const&) { std::exit(0); }, "path");
	p.parse();

	return 1;
}

TEST(empty_args) {
	char* __args[] = { nullptr };
	auto const args = ::args::from_main(0, __args);
	constexpr auto expected_prog_name = ""sv;
	constexpr auto expected_args = 0u;

	EQ(expected_prog_name, args.progname);
	EQ(expected_args, args.args.size());

	return 0;
}

TEST(additional_ctors) {
	char arg0[] = "args-help-test";
	char arg1[] = "--path";
	char arg2[] = "value";
	char* __args[] = { arg0, arg1, arg2, nullptr };
	int argc = static_cast<int>(std::size(__args)) - 1;

	args::null_translator tr;
	args::parser p1{ ""s, arg0, {argc - 1, __args + 1}, &tr };
	args::parser p2{ ""s, args::arglist{argc, __args}, &tr };

	return 0;
}