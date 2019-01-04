// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <args/parser.hpp>

#include <cstring>

std::string_view args::arglist::program_name(std::string_view arg0) noexcept
{
#ifdef _WIN32
	static constexpr char DIRSEP = '\\';
#else
	static constexpr char DIRSEP = '/';
#endif

	auto const pos = arg0.rfind(DIRSEP);
	if (pos != std::string_view::npos)
		arg0 = arg0.substr(pos + 1);

#ifdef _WIN32
	auto const ext = arg0.rfind('.');
	if (ext != std::string_view::npos && ext > 0)
		arg0 = arg0.substr(0, ext);
#endif

	return arg0;
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
		auto count = size_t{};
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

void args::parser::short_help(FILE* out, [[maybe_unused]] bool for_error, std::optional<size_t> maybe_width)
{
	auto shrt{ _(lng::usage) };
	printer_append_usage(shrt);

	printer{ out }.format_paragraph(shrt, 7, maybe_width);
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

void args::parser::usage(std::string_view value)
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

args::arglist args::parser::parse(unknown_action on_unknown)
{
	auto count = args_.size();
	for (decltype(count) i = 0; i < count; ++i) {
		auto arg = args_[i];
		if (arg.length() > 1 && arg[0] == '-') {
			if (arg.length() > 2 && arg[1] == '-') {
				if (!parse_long(arg.substr(2), i, on_unknown))
					return args_.shift(i);
			} else {
				if (!parse_short(arg.substr(1), i, on_unknown))
					return args_.shift(i);
			}
		} else {
			if (!parse_positional(arg, on_unknown))
				return args_.shift(i);
		}
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
			error(_(lng::required, arg));
		}
	}

	return {};
}

bool args::parser::parse_long(const std::string_view& name, int& i, unknown_action on_unknown) {
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

		return true;
	}

	if (on_unknown == exclusive_parser)
		error(_(lng::unrecognized, "--" + s(name)));
	return false;
}

static inline std::string expand(char c) {
	char buff[] = { '-', c, 0 };
	return buff;
}
bool args::parser::parse_short(const std::string_view& name, int& arg, unknown_action on_unknown)
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

		if (!found) {
			if (on_unknown == exclusive_parser)
				error(_(lng::unrecognized, expand(c)));
			return false;
		}
	}

	return true;
}

bool args::parser::parse_positional(const std::string_view& value, unknown_action on_unknown)
{
	for (auto& action : actions_) {
		if (!action->names().empty())
			continue;

		action->visit(*this, s(value));
		return true;
	}

	if (on_unknown == exclusive_parser)
		error(_(lng::unrecognized, value));
	return false;
}
