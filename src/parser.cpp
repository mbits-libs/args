// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <args/parser.hpp>

#include <cstring>
#include <fstream>

namespace {
	inline std::string s(std::string_view sv) {
		return {sv.data(), sv.length()};
	}

	inline std::string const& s(std::string const& in) { return in; }

	inline args::chunk& make_title(args::chunk& part,
	                               std::string title,
	                               size_t count) {
		part.title = std::move(title);
		part.items.reserve(count);
		return part;
	}

	inline std::string to_name(std::string_view key) {
		std::string name;
		if (key.size() == 1) {
			name.reserve(2);
			name.push_back('-');
		} else {
			name.reserve(2 + key.size());
			name.push_back('-');
			name.push_back('-');
		}
		name.append(key);

		return name;
	}
	inline std::string to_name(char key) {
		return to_name(std::string_view(&key, 1));
	}

	using actions_type = std::vector<std::unique_ptr<args::actions::action>>;
	inline args::actions::action* find_by_name(std::string_view name,
	                                           actions_type const& actions) {
		for (auto& action : actions) {
			if (!action->is(name)) continue;
			return action.get();
		}

		return nullptr;
	}

	inline args::actions::action* find_by_name(char name,
	                                           actions_type const& actions) {
		for (auto& action : actions) {
			if (!action->is(name)) continue;
			return action.get();
		}

		return nullptr;
	}

	struct args_list {
		args::arglist args;
		unsigned index{};
		std::string_view current_value{};

		using string_type = std::string_view;

		bool next() {
			if (args.size() == index) return false;
			++index;
			return true;
		}

		std::string_view argument() const noexcept { return args[index - 1]; }

		void set_current(std::string_view curr) { current_value = curr; }
		std::string_view current() const noexcept { return current_value; }

		args::arglist unused() const noexcept {
			auto ndx = index;
			if (ndx) --ndx;
			return args.shift(ndx);
		}
	};

	struct answer_file {
		std::ifstream options{};
		std::string current_line{};
		std::string_view current_value{};

		using string_type = std::string*;

		bool next() {
			bool whitespace_only = true;
			while (whitespace_only) {
				if (!std::getline(options, current_line)) return false;

				for (auto c : current_line) {
					if (!std::isspace(static_cast<unsigned char>(c))) {
						whitespace_only = false;
						break;
					}
				}
			}
			return true;
		}

		std::string const& argument() const noexcept { return current_line; }

		void set_current(std::string_view curr) { current_value = curr; }
		std::string_view current() const noexcept { return current_value; }
	};
}  // namespace

std::string_view args::arglist::program_name(std::string_view arg0) noexcept {
#ifdef _WIN32
	static constexpr char DIRSEP = '\\';
#else
	static constexpr char DIRSEP = '/';
#endif

	auto const pos = arg0.rfind(DIRSEP);
	if (pos != std::string_view::npos) arg0 = arg0.substr(pos + 1);

#ifdef _WIN32
	auto const ext = arg0.rfind('.');
	if (ext != std::string_view::npos && ext > 0) arg0 = arg0.substr(0, ext);
#endif

	return arg0;
}

std::pair<size_t, size_t> args::parser::count_args() const noexcept {
	size_t positionals = 0;
	size_t arguments = provide_help_ ? 1 : 0;

	for (auto& action : actions_) {
		if (action->names().empty())
			++positionals;
		else
			++arguments;
	}

	return {positionals, arguments};
}

void args::parser::printer_append_usage(std::string& shrt) const {
	shrt.append(prog_);

	if (!usage_.empty()) {
		shrt.push_back(' ');
		shrt.append(usage_);
	} else {
		if (provide_help_) shrt.append(" [-h]");
		for (auto& action : actions_)
			action->append_short_help(*tr_, shrt);
	}
}

args::fmt_list args::parser::printer_arguments() const {
	auto [positionals, arguments] = count_args();
	fmt_list info([](auto positionals, auto arguments) {
		auto count = size_t{};
		if (positionals) ++count;
		if (arguments) ++count;
		return count;
	}(positionals, arguments));

	size_t args_id = 0;
	if (positionals)
		make_title(info[args_id++], _(lng::positionals), positionals);

	if (arguments) {
		auto& args = make_title(info[args_id], _(lng::optionals), arguments);
		if (provide_help_)
			args.items.push_back(
			    std::make_pair("-h, --help", _(lng::help_description)));
	}

	for (auto& action : actions_) {
		info[action->names().empty() ? 0 : args_id].items.push_back(
		    std::make_pair(action->help_name(*tr_), action->help()));
	}

	return info;
}

void args::parser::short_help(FILE* out,
                              [[maybe_unused]] bool for_error,
                              std::optional<size_t> maybe_width) const {
	auto shrt{_(lng::usage)};
	printer_append_usage(shrt);

	printer{out}.format_paragraph(shrt, 7, maybe_width);
}

void args::parser::help(std::optional<size_t> maybe_width) const {
	short_help(stdout, false, maybe_width);

	if (!description_.empty()) {
		fputc('\n', stdout);
		printer{stdout}.format_paragraph(description_, 0, maybe_width);
	}

	printer{stdout}.format_list(printer_arguments(), maybe_width);

	std::exit(0);
}

void args::parser::error(std::string const& msg,
                         std::optional<size_t> maybe_width) const {
	short_help(stderr, true, maybe_width);
	printer{stderr}.format_paragraph(_(lng::error_msg, prog_, msg), 0,
	                                 maybe_width);
	std::exit(2);
}

void args::parser::program(std::string const& value) {
	prog_ = value;
}

std::string const& args::parser::program() const noexcept {
	return prog_;
}

void args::parser::usage(std::string_view value) {
	usage_ = value;
}

std::string const& args::parser::usage() const noexcept {
	return usage_;
}

args::arglist args::parser::parse(unknown_action on_unknown,
                                  std::optional<size_t> maybe_width) {
	parse_width_ = maybe_width;
	args_list list{args_};

	if (!parse_list(list, on_unknown)) return list.unused();

	for (auto& action : actions_) {
		if (action->required() && !action->visited()) {
			std::string arg;
			if (action->names().empty())
				arg = action->meta(*tr_);
			else
				arg = to_name(action->names().front());

			error(_(lng::required, arg), maybe_width);
		}
	}

	return {};
}

template <typename ArgList>
bool args::parser::parse_list(ArgList& list, unknown_action on_unknown) {
	while (list.next()) {
		auto&& arg = list.argument();
		if (arg.length() > 1 && arg[0] == '-') {
			if (arg.length() > 2 && arg[1] == '-') {
				list.set_current(std::string_view{arg}.substr(2));
				if (!parse_long(list, on_unknown)) return false;
				continue;
			}

			list.set_current(std::string_view{arg}.substr(1));
			if (!parse_short(list, on_unknown)) return false;
			continue;
		}

		if (uses_answer_file() && arg.length() > 1 &&
		    arg[0] == answer_file_marker()) {
			if (!parse_answer_file(s(arg).substr(1), on_unknown)) return false;
		} else {
			if (!parse_positional(s(arg), on_unknown)) return false;
		}
	}

	return true;
}

template <typename ArgList>
bool args::parser::parse_long(ArgList& list, unknown_action on_unknown) {
	auto name = list.current();

	if (provide_help_ && name == "help") help(parse_width_);

	auto pos = name.find('=');
	auto const name_has_value = pos != std::string_view::npos;
	auto const used_name = name.substr(0, pos);

	auto action = find_by_name(used_name, actions_);

	if (!action) {
		if (on_unknown == exclusive_parser)
			error(_(lng::unrecognized, to_name(used_name)), parse_width_);
		return false;
	}

	if (!action->needs_arg()) {
		if (name_has_value)
			error(_(lng::needs_no_param, to_name(used_name)), parse_width_);

		action->visit(*this);
		return true;
	}

	if (name_has_value) {
		action->visit(*this, s(name.substr(pos + 1)));
		return true;
	}

	if (list.next()) {
		action->visit(*this, s(list.argument()));
		return true;
	}

	error(_(lng::needs_param, to_name(used_name)), parse_width_);
}

template <typename ArgList>
bool args::parser::parse_short(ArgList& list, unknown_action on_unknown) {
	auto argument = list.current();
	auto length = argument.length();
	for (decltype(length) index = 0; index < length; ++index) {
		auto name = argument[index];
		if (provide_help_ && name == 'h') help(parse_width_);

		auto action = find_by_name(name, actions_);

		if (!action) {
			if (on_unknown == exclusive_parser)
				error(_(lng::unrecognized, to_name(name)), parse_width_);
			return false;
		}

		if (!action->needs_arg()) {
			action->visit(*this);
			continue;
		}

		std::string param;

		++index;
		if (index < length) {
			auto param = argument.substr(index);
			index = length;
			action->visit(*this, s(param));
			continue;
		}

		if (list.next()) {
			action->visit(*this, s(list.argument()));
			continue;
		}

		error(_(lng::needs_param, to_name(name)), parse_width_);
	}

	return true;
}

bool args::parser::parse_positional(std::string const& value,
                                    unknown_action on_unknown) {
	for (auto& action : actions_) {
		if (!action->names().empty()) continue;

		action->visit(*this, value);
		return true;
	}

	if (on_unknown == exclusive_parser)
		error(_(lng::unrecognized, value), parse_width_);
	return false;
}

bool args::parser::parse_answer_file(std::string const& path,
                                     unknown_action on_unknown) {
	answer_file list{};
	list.options.open(path);
	if (list.options.fail()) error(_(lng::file_not_found, path), parse_width_);
	return parse_list(list, on_unknown);
}
