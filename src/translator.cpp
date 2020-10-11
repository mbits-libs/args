// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <args/translator.hpp>

args::base_translator::~base_translator() = default;
args::base_translator::base_translator() = default;

static std::string s(std::string_view sv) {
	return {sv.data(), sv.length()};
}

std::string args::null_translator::operator()(lng id,
                                              std::string_view arg1,
                                              std::string_view arg2) const {
	switch (id) {
		case lng::usage:
			return "usage: ";
		case lng::def_meta:
			return "ARG";
		case lng::positionals:
			return "positional arguments";
		case lng::optionals:
			return "optional arguments";
		case lng::help_description:
			return "show this help message and exit";
		case lng::unrecognized:
			return "unrecognized argument: " + s(arg1);
		case lng::needs_param:
			return "argument " + s(arg1) + ": expected one argument";
		case lng::needs_number:
			return "argument " + s(arg1) + ": expected a number";
		case lng::needed_number_exceeded:
			return "argument " + s(arg1) +
			       ": number outside of expected bounds";
		case lng::needed_enum_unknown:
			return "argument " + s(arg1) + ": value " + s(arg2) +
			       " is not recognized";
		case lng::needed_enum_known_values:
			return "known values for " + s(arg1) + ": " + s(arg2);
		case lng::required:
			return "argument " + s(arg1) + " is required";
		case lng::error_msg:
			return s(arg1) + ": error: " + s(arg2);
	}
	return "<unrecognized string>";
}
