/*
 * Copyright (C) 2015 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <args/argparser.hpp>

#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#	include <io.h>
#	include <windows.h>
#else
#	include <sys/ioctl.h>
#	include <unistd.h>
#	define _isatty(FD) isatty(FD)
#	define _fileno(OBJ) fileno(OBJ)
#endif

bool args::detail::is_terminal(FILE* out) noexcept {
	return !!_isatty(_fileno(out));
}

int args::detail::terminal_width(FILE* out) noexcept {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO buffer = { sizeof(CONSOLE_SCREEN_BUFFER_INFO) };
	auto handle = (HANDLE)_get_osfhandle(_fileno(out));
	if (handle == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(handle, &buffer))
		return 0;
	return buffer.dwSize.X;
#else
	winsize w;
	while (ioctl(fileno(out), TIOCGWINSZ, &w) == -1) {
		if (errno != EINTR)
			return 0;
	}
	return w.ws_col;
#endif
}

args::actions::action::~action() = default;
args::actions::action::action() = default;
args::actions::action::action(action&&) = default;
args::actions::action& args::actions::action::operator=(action&&) = default;

void args::actions::action::append_short_help(const base_translator& _, std::string& s) const
{
	auto aname = ([this](const base_translator& _) {
		if (names().empty())
			return meta(_);

		const auto& name = names().front();
		const size_t length = name.length();
		size_t additional = 0;
		if (length > 1)
			++additional;
		++additional;
		std::string aname;
		aname.reserve(length + additional);
		aname.push_back('-');
		if (length > 1)
			aname.push_back('-');
		aname.append(name);

		if (needs_arg()) {
			aname.push_back(' ');
			aname.append(meta(_));
		}

		return aname;
	}(_));

	int flags =
		(required() ? 2 : 0) |
		(multiple() ? 1 : 0);

	if (flags & 2) {
		s.push_back(' ');
		s.append(aname);
	}

	if (flags & 1) {
		s.append(" [");
		s.append(aname);
		s.append(" ...]");
	}

	if (!flags) {
		s.append(" [");
		s.append(aname);
		s.push_back(']');
	}
}

std::string args::actions::action::help_name(const base_translator& _) const
{
	auto meta_value = meta(_);

	size_t length = 0;
	bool first = true;
	for (auto& name : names()) {
		if (first) first = false;
		else length += 2;

		++length;
		if (name.length() > 1)
			++length;
		length += name.length();
	}

	if (!length)
		return meta_value;

	if (needs_arg())
		length += 1 + meta_value.length();

	std::string nmz;
	nmz.reserve(length);

	for (auto& name : names()) {
		if (first) first = false;
		else nmz.append(", ");

		if (name.length() > 1)
			nmz.append("--");
		else
			nmz.append("-");
		nmz.append(name);
	}

	if (needs_arg()) {
		nmz.push_back(' ');
		nmz.append(meta(_));
	}

	return nmz;
}

std::string args::parser::program_name(const char* arg0) noexcept
{
#ifdef _WIN32
	static constexpr char DIRSEP = '\\';
#else
	static constexpr char DIRSEP = '/';
#endif

	auto program = std::strrchr(arg0, DIRSEP);
	if (program) ++program;
	else program = arg0;

#ifdef _WIN32
	auto ext = std::strrchr(program, '.');
	if (ext && ext != program)
		return { program, ext };
#endif

	return program;
}

std::pair<size_t, size_t> args::parser::count_args() const noexcept
{
	size_t positionals = 0;
	size_t arguments = provide_help_ ? 1 : 0;

	for (auto& action : actions_) {
		if (action->names().empty())
			++positionals;
		else
			++arguments;
	}

	return { positionals, arguments };
}

void args::parser::printer_append_usage(std::string& shrt) const {
	shrt.append(prog_);

	if (!usage_.empty()) {
		shrt.push_back(' ');
		shrt.append(usage_);
	}
	else {
		if (provide_help_)
			shrt.append(" [-h]");
		for (auto& action : actions_)
			action->append_short_help(*tr_, shrt);
	}
}

static args::chunk& make_title(args::chunk& part, std::string title, size_t count)
{
	part.title = std::move(title);
	part.items.reserve(count);
	return part;
}

args::fmt_list args::parser::printer_arguments() const {
	auto[positionals, arguments] = count_args();
	fmt_list info([](auto positionals, auto arguments) {
		auto count = 0;
		if (positionals)
			++count;
		if (arguments)
			++count;
		return count;
	}(positionals, arguments));

	size_t args_id = 0;
	if (positionals)
		make_title(info[args_id++], _(lng::positionals), positionals);

	if (arguments) {
		auto& args = make_title(info[args_id], _(lng::optionals), arguments);
		if (provide_help_)
			args.items.push_back(std::make_pair("-h, --help", _(lng::help_description)));
	}

	for (auto& action : actions_) {
		info[action->names().empty() ? 0 : args_id].items
			.push_back(std::make_pair(action->help_name(*tr_), action->help()));
	}

	return info;
}

void args::parser::short_help(FILE* out, bool for_error, std::optional<size_t> maybe_width)
{
	auto shrt{ _(lng::usage) };
	printer_append_usage(shrt);

	printer{ out }.format_paragraph(shrt, 7);
}

void args::parser::help(std::optional<size_t> maybe_width)
{
	short_help(stdout, false, maybe_width);

	if (!description_.empty()) {
		fputc('\n', stdout);
		printer{ stdout }.format_paragraph(description_, 0, maybe_width);
	}

	printer{ stdout }.format_list(printer_arguments(), maybe_width);

	std::exit(0);
}

void args::parser::error(const std::string& msg, std::optional<size_t> maybe_width)
{
	short_help(stderr, true, maybe_width);
	printer{ stderr }.format_paragraph(_(lng::error_msg, prog_, std::move(msg)), 0, maybe_width);
	std::exit(2);
}

void args::parser::program(const std::string& value)
{
	prog_ = value;
}

const std::string& args::parser::program()
{
	return prog_;
}

void args::parser::usage(const std::string& value)
{
	usage_ = value;
}

const std::string& args::parser::usage()
{
	return usage_;
}

static std::string s(std::string_view sv) {
	return { sv.data(), sv.length() };
}

void args::parser::parse()
{
	auto count = args_.size();
	for (decltype(count) i = 0; i < count; ++i) {
		auto& arg = args_[i];
		if (arg.length() > 1 && arg[0] == '-') {
			if (arg.length() > 2 && arg[1] == '-')
				parse_long(arg.substr(2), i);
			else
				parse_short(arg.substr(1), i);
		} else
			parse_positional(arg);
	}

	for (auto& action : actions_) {
		if (action->required() && !action->visited()) {
			std::string arg;
			if (action->names().empty()) {
				arg = action->meta(*tr_);
			} else {
				auto& name = action->names().front();
				arg = name.length() == 1 ? "-" + name : "--" + name;
			}
			error(_(lng::requires, arg));
		}
	}
}

void args::parser::parse_long(const std::string_view& name, size_t& i) {
	if (provide_help_ && name == "help")
		help();

	for (auto& action : actions_) {
		if (!action->is(name))
			continue;

		if (action->needs_arg()) {
			++i;
			if (i >= args_.size())
				error(_(lng::needs_param, "--" + s(name)));

			action->visit(*this, s(args_[i]));
		}
		else
			action->visit(*this);

		return;
	}

	error(_(lng::unrecognized, "--" + s(name)));
}

static inline std::string expand(char c) {
	char buff[] = { '-', c, 0 };
	return buff;
}
void args::parser::parse_short(const std::string_view& name, size_t& arg)
{
	auto length = name.length();
	for (decltype(length) i = 0; i < length; ++i) {
		auto c = name[i];
		if (provide_help_ && c == 'h')
			help();

		bool found = false;
		for (auto& action : actions_) {
			if (!action->is(c))
				continue;

			if (action->needs_arg()) {
				std::string param;

				++i;
				if (i < length)
					param = name.substr(i);
				else {
					++arg;
					if (arg >= args_.size())
						error(_(lng::needs_param, expand(c)));

					param = args_[arg];
				}

				i = length;

				action->visit(*this, param);
			}
			else
				action->visit(*this);

			found = true;
			break;
		}

		if (!found)
			error(_(lng::unrecognized, expand(c)));
	}
}

void args::parser::parse_positional(const std::string_view& value)
{
	for (auto& action : actions_) {
		if (!action->names().empty())
			continue;

		action->visit(*this, s(value));
		return;
	}

	error(_(lng::unrecognized, value));
}
