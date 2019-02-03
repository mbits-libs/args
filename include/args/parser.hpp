// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <args/actions.hpp>
#include <args/translator.hpp>
#include <args/printer.hpp>

#include <memory>

namespace args {
	class arglist {
		int count_{};
		char** args_{};
	public:
		constexpr arglist() = default;
		constexpr arglist(arglist const&) = default;
		constexpr arglist& operator=(arglist const&) = default;
		constexpr arglist(arglist&&) = default;
		constexpr arglist& operator=(arglist&&) = default;

		constexpr arglist(int argc, char* argv[])
			: count_{ argc }, args_{ argv }
		{}

		constexpr bool empty() const noexcept { return !count_; }
		constexpr int size() const noexcept { return count_; }
		constexpr char* const* data() const noexcept { return args_; }

		constexpr std::string_view operator[](int i) const noexcept { return args_[i]; }
		constexpr arglist shift(int n = 1) const noexcept {
			if (n < 0) {
				n = -n;
				if (n > count_)
					n = count_;
				return { count_ - n, args_ };
			}
			if (n >= count_)
				n = count_;
			return { count_ - n, args_ + n };
		}

		static std::string_view program_name(std::string_view arg0) noexcept;
	};

	struct args_view {
		std::string_view progname{};
		arglist args{};
	};

	inline args_view from_main(arglist const& args) noexcept {
		if (args.empty())
			return {};
		auto const progname = arglist::program_name(args[0]);
		return { progname, args.shift() };
	}

	inline args_view from_main(int argc, char* argv[]) noexcept {
		return from_main({ argc, argv });
	}

	class parser {
	public:
		enum unknown_action {
			exclusive_parser = 0,
			allow_subcommands = 1
		};

	private:
		std::vector<std::unique_ptr<actions::action>> actions_;
		std::string description_;
		arglist args_;
		std::string prog_;
		std::string usage_;
		bool provide_help_ = true;
		const base_translator* tr_;
		std::string _(lng id, std::string_view arg1 = { }, std::string_view arg2 = { }) const {
			return (*tr_)(id, arg1, arg2);
		}

		std::pair<size_t, size_t> count_args() const noexcept;

		bool parse_long(const std::string_view& name, int& i, unknown_action on_unknown);
		bool parse_short(const std::string_view& name, int& i, unknown_action on_unknown);
		bool parse_positional(const std::string_view& value, unknown_action on_unknown);

		template <typename T, typename... Args>
		actions::builder add(Args&&... args)
		{
			actions_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
			return { actions_.back().get(), true };
		}

		template <typename T, typename... Args>
		actions::builder add_opt(Args&&... args)
		{
			actions_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
			return { actions_.back().get(), false };
		}

	public:
		parser(std::string description, args_view const& args, const base_translator* tr)
			: description_{ std::move(description) }
			, args_{ args.args }
			, prog_{ args.progname }
			, tr_{ tr }
		{
		}

		parser(std::string description, std::string_view progname, arglist const& args, const base_translator* tr)
			: parser(std::move(description), { progname, args }, tr)
		{}

		parser(std::string description, arglist const& args, const base_translator* tr)
			: parser(std::move(description), from_main(args), tr)
		{}

		template <typename T, typename... Names>
		actions::builder arg(T& dst, Names&&... names) {
			return add<actions::store_action<T>>(&dst, std::forward<Names>(names)...);
		}

		template <typename T, typename... Names>
		actions::builder arg(std::optional<T>& dst, Names&&... names) {
			return add_opt<actions::store_action<std::optional<T>>>(&dst, std::forward<Names>(names)...);
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

		void usage(std::string_view value);
		const std::string& usage();

		void provide_help(bool value = true) { provide_help_ = value; }
		bool provide_help() { return provide_help_; }

		arglist const& args() const { return args_; }

		const base_translator& tr() const noexcept { return *tr_; }

		arglist parse(unknown_action on_unknown = exclusive_parser);

		void printer_append_usage(std::string& out) const;
		fmt_list printer_arguments() const;

		void short_help(FILE* out = stdout, bool for_error = false,
			std::optional<size_t> maybe_width = {});
		[[noreturn]] void help(std::optional<size_t> maybe_width = {});
		[[noreturn]] void error(const std::string& msg,
			std::optional<size_t> maybe_width = {});
	};
}
