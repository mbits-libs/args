// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <args/actions.hpp>
#include <args/translator.hpp>
#include <args/printer.hpp>

#include <memory>

namespace args {
	class parser {
		std::vector<std::unique_ptr<actions::action>> actions_;
		std::string description_;
		std::vector<std::string_view> args_;
		std::string prog_;
		std::string usage_;
		bool provide_help_ = true;
		const base_translator* tr_;
		std::string _(lng id, std::string_view arg1 = { }, std::string_view arg2 = { }) const {
			return (*tr_)(id, arg1, arg2);
		}

		static std::string program_name(const char* arg0) noexcept;
		std::pair<size_t, size_t> count_args() const noexcept;

		void parse_long(const std::string_view& name, size_t& i);
		void parse_short(const std::string_view& name, size_t& i);
		void parse_positional(const std::string_view& value);

		template <typename T, typename... Args>
		actions::builder add(Args&&... args)
		{
			actions_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
			return actions_.back().get();
		}
	public:
		parser(const std::string& description, int argc, char* argv[], const base_translator* tr)
			: description_{ description }
			, prog_{ program_name(argv[0]) }
			, tr_{ tr }
		{
			args_.reserve(argc - 1);
			for (int i = 1; i < argc; ++i)
				args_.emplace_back(argv[i]);
		}

		template <typename T, typename... Names>
		actions::builder arg(T& dst, Names&&... names) {
			return add<actions::store_action<T>>(&dst, std::forward<Names>(names)...);
		}

		template <typename Value, typename T, typename... Names>
		actions::builder set(T& dst, Names&&... names) {
			return add<actions::set_value<T, Value>>(&dst, std::forward<Names>(names)...);
		}

		template <typename Callable, typename... Names>
		std::enable_if_t<
			actions::detail::is_compatible_with_v<Callable> ||
			actions::detail::is_compatible_with_v<Callable, const std::string&>,
			actions::builder>
		custom(Callable cb, Names&&... names)
		{
			return add<actions::custom_action<Callable>>(std::move(cb), std::forward<Names>(names)...);
		}

		void program(const std::string& value);
		const std::string& program();

		void usage(const std::string& value);
		const std::string& usage();

		void provide_help(bool value = true) { provide_help_ = value; }
		bool provide_help() { return provide_help_; }

		const std::vector<std::string_view>& args() const { return args_; }

		void parse();

		void printer_append_usage(std::string& out) const;
		fmt_list printer_arguments() const;

		void short_help(FILE* out = stdout, bool for_error = false,
			std::optional<size_t> maybe_width = {});
		[[noreturn]] void help(std::optional<size_t> maybe_width = {});
		[[noreturn]] void error(const std::string& msg,
			std::optional<size_t> maybe_width = {});
	};
}
