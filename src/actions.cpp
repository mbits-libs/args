// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <args/actions.hpp>
#include <args/translator.hpp>

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

std::string args::actions::action_base::meta(const base_translator& _) const {
	return meta_.empty() ? _(lng::def_meta) : meta_;
}
