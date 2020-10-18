// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <args/actions.hpp>
#include <args/printer.hpp>
#include <args/translator.hpp>
#include <args/version.hpp>

#include <memory>

namespace args {
	class arglist {
		unsigned count_{};
		char** args_{};

	public:
		constexpr arglist() = default;
		constexpr arglist(arglist const&) = default;
		constexpr arglist& operator=(arglist const&) = default;
		constexpr arglist(arglist&&) = default;
		constexpr arglist& operator=(arglist&&) = default;

		constexpr arglist(int argc, char* argv[])
		    : count_{argc < 0 ? 0u : static_cast<unsigned>(argc)}
		    , args_{argv} {}

		constexpr arglist(unsigned argc, char* argv[])
		    : count_{argc}, args_{argv} {}

		constexpr bool empty() const noexcept { return !count_; }
		constexpr unsigned size() const noexcept { return count_; }
		constexpr char* const* data() const noexcept { return args_; }

		constexpr std::string_view operator[](unsigned i) const noexcept {
			return args_[i];
		}
		constexpr arglist shift(unsigned n = 1) const noexcept {
			if (n >= count_) n = count_;
			return {count_ - n, args_ + n};
		}

		LIBARGS_API static std::string_view program_name(
		    std::string_view arg0) noexcept;
	};

	struct args_view {
		std::string_view progname{};
		arglist args{};
	};

	inline args_view from_main(arglist const& args) noexcept {
		if (args.empty()) return {};
		auto const progname = arglist::program_name(args[0]);
		return {progname, args.shift()};
	}

	inline args_view from_main(int argc, char* argv[]) noexcept {
		return from_main({argc, argv});
	}

#if defined(HAS_STD_CONCEPTS)
	// can I build a string out of it?
	template <typename NameType>
	concept StringLike = std::constructible_from<std::string, NameType>;

	template <typename Callable>
	concept AnyActionHandler =
	    actions::detail::ActionHandler<Callable> ||
	    actions::detail::ActionHandler<Callable, std::string const&>;
#else
	template <typename Callable>
	constexpr bool is_any_action_handler_v =
	    actions::detail::is_action_handler_v<Callable> ||
	    actions::detail::is_action_handler_v<Callable, std::string const&>;
#endif

	class parser {
	public:
		enum unknown_action { exclusive_parser = 0, allow_subcommands = 1 };

	private:
		std::vector<std::unique_ptr<actions::action>> actions_;
		std::string description_;
		arglist args_;
		std::string prog_;
		std::string usage_;
		bool provide_help_ = true;
		char answer_file_marker_{};
		std::optional<size_t> parse_width_ = {};
		base_translator const* tr_;
		[[nodiscard]] std::string _(lng id,
		                            std::string_view arg1 = {},
		                            std::string_view arg2 = {}) const {
			return (*tr_)(id, arg1, arg2);
		}

		[[nodiscard]] std::pair<size_t, size_t> count_args() const noexcept;

		template <typename ArgList>
		bool parse_list(ArgList& list, unknown_action on_unknown);
		template <typename ArgList>
		bool parse_long(ArgList& list, unknown_action on_unknown);
		template <typename ArgList>
		bool parse_short(ArgList& list, unknown_action on_unknown);
		bool parse_positional(std::string const& value,
		                      unknown_action on_unknown);
		bool parse_answer_file(std::string const& value,
		                       unknown_action on_unknown);

		template <typename Action, typename... Args>
		actions::builder add(Args&&... args) {
			actions_.push_back(
			    std::make_unique<Action>(std::forward<Args>(args)...));
			return {actions_.back().get(), true};
		}

		template <typename Action, typename... Args>
		actions::builder add_opt(Args&&... args) {
			actions_.push_back(
			    std::make_unique<Action>(std::forward<Args>(args)...));
			return {actions_.back().get(), false};
		}

	public:
		parser(std::string description,
		       args_view const& args,
		       base_translator const* tr)
		    : description_{std::move(description)}
		    , args_{args.args}
		    , prog_{args.progname}
		    , tr_{tr} {}

		parser(std::string description,
		       std::string_view progname,
		       arglist const& args,
		       base_translator const* tr)
		    : parser(std::move(description), {progname, args}, tr) {}

		parser(std::string description,
		       arglist const& args,
		       base_translator const* tr)
		    : parser(std::move(description), from_main(args), tr) {}

		template <typename Storage, typename... Names>
#if defined(HAS_STD_CONCEPTS)
		requires(StringLike<Names>&&...)
#endif
		    actions::builder arg(Storage& dst, Names&&... names) {
			return add<actions::store_action<Storage>>(
			    &dst, std::forward<Names>(names)...);
		}

		template <typename Storage, typename... Names>
#if defined(HAS_STD_CONCEPTS)
		requires(StringLike<Names>&&...)
#endif
		    actions::builder
		    arg(std::optional<Storage>& dst, Names&&... names) {
			return add_opt<actions::store_action<std::optional<Storage>>>(
			    &dst, std::forward<Names>(names)...);
		}

		template <typename Value, typename Storage, typename... Names>
#if defined(HAS_STD_CONCEPTS)
		requires((StringLike<Names> && ...) &&
		         requires() {
			         { Value::value }
			         ->std::convertible_to<Storage>;
		         })
#endif
		    actions::builder set(Storage& dst, Names&&... names) {
			return add<actions::set_value<Storage, Value>>(
			    &dst, std::forward<Names>(names)...);
		}

		template <typename Callable, typename... Names>
#if defined(HAS_STD_CONCEPTS)
		requires(AnyActionHandler<Callable> &&
		         (StringLike<Names> && ...)) actions::builder
#else
		std::enable_if_t<is_any_action_handler_v<Callable>, actions::builder>
#endif
		    custom(Callable cb, Names&&... names) {
			return add<actions::custom_action<Callable>>(
			    std::move(cb), std::forward<Names>(names)...);
		}

		LIBARGS_API void program(std::string const& value);
		LIBARGS_API std::string const& program() const noexcept;

		LIBARGS_API void usage(std::string_view value);
		LIBARGS_API std::string const& usage() const noexcept;

		void provide_help(bool value = true) { provide_help_ = value; }
		bool provides_help() const noexcept { return provide_help_; }

		void use_answer_file(char marker = '@') {
			answer_file_marker_ = marker;
		}
		bool uses_answer_file() const noexcept {
			return answer_file_marker_ != 0;
		}
		char answer_file_marker() const noexcept { return answer_file_marker_; }

		arglist const& args() const noexcept { return args_; }

		base_translator const& tr() const noexcept { return *tr_; }

		std::optional<size_t> parse_width() const noexcept {
			return parse_width_;
		}

		LIBARGS_API arglist parse(unknown_action on_unknown = exclusive_parser,
		                          std::optional<size_t> maybe_width = {});

		LIBARGS_API void printer_append_usage(std::string& out) const;
		LIBARGS_API fmt_list printer_arguments() const;

		LIBARGS_API void short_help(
		    FILE* out = stdout,
		    bool for_error = false,
		    std::optional<size_t> maybe_width = {}) const;
		[[noreturn]] LIBARGS_API void help(
		    std::optional<size_t> maybe_width = {}) const;
		[[noreturn]] LIBARGS_API void error(
		    std::string const& msg,
		    std::optional<size_t> maybe_width = {}) const;
	};
}  // namespace args
