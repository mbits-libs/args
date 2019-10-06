// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <string>
#include <string_view>

namespace args {
	enum class lng : int {
		usage,
		def_meta,
		positionals,
		optionals,
		help_description,
		unrecognized,
		needs_param,
		needs_number,
		needed_number_exceeded,
		required,
		error_msg
	};

	struct base_translator {
		virtual ~base_translator();
		base_translator();
		base_translator(const base_translator&) = delete;
		base_translator(base_translator&&) = delete;
		base_translator& operator=(const base_translator&) = delete;
		base_translator& operator=(base_translator&&) = delete;
		virtual std::string operator()(lng id, std::string_view arg1 = { }, std::string_view arg2 = { }) const = 0;
	};

	class null_translator : public base_translator {
	public:
		std::string operator()(lng id, std::string_view arg1, std::string_view arg2) const override;
	};
}