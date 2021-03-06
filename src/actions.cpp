// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <args/actions.hpp>
#include <args/parser.hpp>
#include <args/translator.hpp>

args::actions::action::~action() = default;
args::actions::action::action() = default;
args::actions::action::action(action&&) = default;
args::actions::action& args::actions::action::operator=(action&&) = default;

void args::actions::action::append_short_help(base_translator const& _,
                                              std::string& s) const {
	auto aname = ([this](base_translator const& _) {
		if (names().empty()) return meta(_);

		auto const& name = names().front();
		size_t const length = name.length();
		size_t additional = 0;
		if (length > 1) ++additional;
		++additional;
		std::string aname;
		aname.reserve(length + additional);
		aname.push_back('-');
		if (length > 1) aname.push_back('-');
		aname.append(name);

		if (needs_arg()) {
			aname.push_back(' ');
			aname.append(meta(_));
		}

		return aname;
	}(_));

	int flags = (required() ? 2 : 0) | (multiple() ? 1 : 0);

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

std::string args::actions::action::help_name(base_translator const& _) const {
	auto meta_value = meta(_);

	size_t length = 0;
	bool first = true;
	for (auto& name : names()) {
		if (first)
			first = false;
		else
			length += 2;

		++length;
		if (name.length() > 1) ++length;
		length += name.length();
	}

	if (!length) return meta_value;

	if (needs_arg()) length += 1 + meta_value.length();

	std::string nmz;
	nmz.reserve(length);

	first = true;
	for (auto& name : names()) {
		if (first)
			first = false;
		else
			nmz.append(", ");

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

std::string args::actions::action_base::meta(base_translator const& _) const {
	return meta_.empty() ? _(lng::def_meta) : meta_;
}

std::string args::actions::action_base::argname(parser& p) const {
	if (names().empty()) return meta(p.tr());
	auto& name = names().front();
	if (name.length() > 1) return "--" + name;
	return "-" + name;
}

[[noreturn]] void args::actions::argument_is_not_integer(
    parser& p,
    std::string const& name) {
	p.error(p.tr()(lng::needs_number, name), p.parse_width());
}

[[noreturn]] void args::actions::argument_out_of_range(
    parser& p,
    std::string const& name) {
	p.error(p.tr()(lng::needed_number_exceeded, name), p.parse_width());
}

[[noreturn]] void args::actions::enum_argument_out_of_range(
    parser& p,
    std::string const& name,
    std::string const& value,
    std::string const& values) {
	p.error(p.tr()(lng::needed_enum_unknown, name, value) + "\n" +
	            p.tr()(lng::needed_enum_known_values, name, values),
	        p.parse_width());
}
