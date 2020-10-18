// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <array>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

#if defined(__cpp_concepts)
#if defined(__has_include)
#if __has_include(<concepts>)
#include <concepts>
#if defined(__cpp_lib_concepts)
#define HAS_STD_CONCEPTS
#endif
#endif
#endif
#endif

#include <args/api.hpp>

namespace args {
	class parser;
	struct base_translator;

	namespace actions {
		[[noreturn]] LIBARGS_API void argument_is_not_integer(
		    parser& p,
		    std::string const& name);
		[[noreturn]] LIBARGS_API void argument_out_of_range(
		    parser& p,
		    std::string const& name);
		[[noreturn]] LIBARGS_API void enum_argument_out_of_range(
		    parser& p,
		    std::string const& name,
		    std::string const& value,
		    std::string const& values);
	}  // namespace actions

	template <typename Storage, typename = void>
	struct converter {};

	template <typename Storage>
	struct string_converter {
		static inline Storage value(parser&,
		                            std::string const& arg,
		                            std::string const&) {
			return arg;
		}
	};

	template <typename Storage>
	struct is_optional : std::false_type {};
	template <typename Storage>
	struct is_optional<std::optional<Storage>> : std::true_type {};
#ifdef __cpp_concepts
	template <typename Storage>
	concept optional = is_optional<Storage>::value;
#endif

	template <typename Storage>
#ifdef HAS_STD_CONCEPTS
	requires(std::constructible_from<Storage, std::string const&> &&
	         !optional<Storage>) struct converter<Storage>
#else
	struct converter<
	    Storage,
	    std::enable_if_t<std::is_constructible_v<Storage, std::string const&> &&
	                     !is_optional<Storage>::value>>
#endif
	    : string_converter<Storage> {
	};

	template <>
	struct converter<std::string_view> {};

	template <typename Storage>
	struct from_chars_converter {
		static inline Storage value(parser& p,
		                            std::string const& arg,
		                            std::string const& name) {
			Storage out{};
			auto first = arg.data();
			auto last = first + arg.length();
			auto const result = std::from_chars(first, last, out);

			if (result.ec == std::errc::result_out_of_range)
				actions::argument_out_of_range(p, name);

			if ((result.ptr && result.ptr != last) || result.ec != std::errc{})
				actions::argument_is_not_integer(p, name);

			return out;
		}
	};

	template <typename Storage>
#ifdef HAS_STD_CONCEPTS
	requires(std::integral<Storage> &&
	         !std::same_as<Storage, bool>) struct converter<Storage>
#else
	struct converter<Storage,
	                 std::enable_if_t<std::is_integral_v<Storage> &&
	                                  !std::is_same_v<Storage, bool>>>
#endif
	    : from_chars_converter<Storage> {
	};

	template <typename Storage>
	struct converter<std::optional<Storage>> {
		static inline std::optional<Storage> value(parser& p,
		                                           std::string const& arg,
		                                           std::string const& name) {
			using inner = converter<Storage>;
			return inner::value(p, arg, name);
		}
	};

	template <typename Storage, typename NamesType>
	struct enum_traits_base {
		using name_info = std::pair<std::string_view, Storage>;
		name_info const* begin() const noexcept {
			return NamesType::names().data();
		}
		name_info const* end() const noexcept {
			return NamesType::names().data() + NamesType::names().size();
		}
	};

	template <typename Storage>
	struct enum_traits;

	template <typename Storage>
	struct names_helper;

// clang-format off
// plz, don't mess with it anymore...
#define ENUM_TRAITS_BEGIN(STORAGE)                                      \
	namespace args {                                                    \
		template <>                                                     \
		struct names_helper<STORAGE> {                                  \
			static inline auto const& names() {                         \
				using enum_stg = STORAGE;                               \
				using name_info = std::pair<std::string_view, STORAGE>; \
				static constexpr std::array enum_names = {
#define ENUM_TRAITS_NAME_EX(VALUE, NAME) name_info{NAME, VALUE},
#define ENUM_TRAITS_NAME(VALUE) name_info{#VALUE, enum_stg::VALUE},
#define ENUM_TRAITS_END(STORAGE)                                             \
				};                                                           \
                                                                             \
				return enum_names;                                           \
			}                                                                \
		};                                                                   \
                                                                             \
		template <>                                                          \
		struct enum_traits<STORAGE>                                          \
			: enum_traits_base<STORAGE, names_helper<STORAGE>> {             \
			using parent = enum_traits_base<STORAGE, names_helper<STORAGE>>; \
			using name_info = typename parent::name_info;                    \
		};                                                                   \
	}
	// clang-format on

	template <typename Storage>
	struct enum_converter {
		static inline Storage value(parser& p,
		                            std::string const& arg,
		                            std::string const& name) {
			using traits = enum_traits<Storage>;
			for (auto [value_name, val] : traits{}) {
				if (value_name == arg) return val;
			}

			std::string values{};
			size_t length{};
			for (auto [value_name, val] : traits{}) {
				length += value_name.size() + 2;
			}
			if (length) length -= 2;
			values.reserve(length);
			bool first = true;
			for (auto [value_name, val] : traits{}) {
				if (first)
					first = false;
				else
					values.append(", ");
				values.append(value_name);
			}

			actions::enum_argument_out_of_range(p, name, arg, values);
		}
	};

	template <typename Storage>
#ifdef HAS_STD_CONCEPTS
	requires std::is_enum_v<Storage> struct converter<Storage>
#else
	struct converter<Storage, std::enable_if_t<std::is_enum_v<Storage>>>
#endif
	    : enum_converter<Storage> {
	};

	namespace actions {
		struct LIBARGS_API action {
			virtual ~action();
			virtual bool required() const = 0;
			virtual void required(bool value) = 0;
			virtual bool multiple() const = 0;
			virtual void multiple(bool value) = 0;
			virtual bool needs_arg() const = 0;
			virtual void visit(parser&) = 0;
			virtual void visit(parser&, std::string const& /*arg*/) = 0;
			virtual bool visited() const = 0;
			virtual void meta(std::string_view s) = 0;
			virtual std::string meta(base_translator const&) const = 0;
			virtual void help(std::string_view s) = 0;
			virtual std::string const& help() const = 0;
			virtual bool is(std::string_view name) const = 0;
			virtual bool is(char name) const = 0;
			virtual std::vector<std::string> const& names() const = 0;

			void append_short_help(base_translator const& _,
			                       std::string& s) const;
			std::string help_name(base_translator const& _) const;

		protected:
			action();
			action(action const&) = delete;
			action(action&&);
			action& operator=(action const&) = delete;
			action& operator=(action&&);
		};

		class builder {
			friend class ::args::parser;

			action* ptr;
			builder(action* ptr, bool required) : ptr(ptr) { req(required); }

		public:
			builder(builder const&) = delete;
			builder(builder&&) = default;
			builder& operator=(builder const&) = delete;
			builder& operator=(builder&&) = default;

			builder& meta(std::string_view name) {
				ptr->meta(name);
				return *this;
			}
			builder& help(std::string_view dscr) {
				ptr->help(dscr);
				return *this;
			}
			builder& multi(bool value = true) {
				ptr->multiple(value);
				return *this;
			}
			builder& req(bool value = true) {
				ptr->required(value);
				return *this;
			}
			builder& opt(bool value = true) {
				ptr->required(!value);
				return *this;
			}
		};

		class action_base : public action {
			std::vector<std::string> names_;
			std::string meta_;
			std::string help_;
			bool visited_ = false;
			bool required_ = true;
			bool multiple_ = false;

			static void pack(std::vector<std::string>&) {}
			template <typename Name, typename... Names>
			static void pack(std::vector<std::string>& dst,
			                 Name&& name,
			                 Names&&... names) {
				dst.emplace_back(std::forward<Name>(name));
				pack(dst, std::forward<Names>(names)...);
			}

		protected:
			template <typename... Names>
			action_base(Names&&... argnames) {
				pack(names_, std::forward<Names>(argnames)...);
			}

			void visited(bool val) { visited_ = val; }

			LIBARGS_API std::string argname(parser&) const;

		public:
			void required(bool value) override { required_ = value; }
			bool required() const override { return required_; }
			void multiple(bool value) override { multiple_ = value; }
			bool multiple() const override { return multiple_; }

			void visit(parser&) override { visited_ = true; }
			void visit(parser&, std::string const& /*arg*/) override {
				visited_ = true;
			}
			bool visited() const override { return visited_; }
			void meta(std::string_view s) override { meta_ = s; }
			LIBARGS_API std::string meta(
			    base_translator const& _) const override;
			void help(std::string_view s) override { help_ = s; }
			std::string const& help() const override { return help_; }

			bool is(std::string_view name) const override {
				for (auto& argname : names_) {
					if (argname.length() > 1 && argname == name) return true;
				}

				return false;
			}

			bool is(char name) const override {
				for (auto& argname : names_) {
					if (argname.length() == 1 && argname[0] == name)
						return true;
				}

				return false;
			}

			std::vector<std::string> const& names() const override {
				return names_;
			}
		};

		template <typename Storage, typename = void*>
		class store_action final : public action_base {
			Storage* ptr;

		public:
			template <typename... Names>
			explicit store_action(Storage* dst, Names&&... names)
			    : action_base(std::forward<Names>(names)...), ptr(dst) {}

			bool needs_arg() const override { return true; }
			using action::visit;
			void visit(parser& p, std::string const& arg) override {
				*ptr = converter<Storage>::value(p, arg, argname(p));
				visited(true);
			}
		};

		template <typename Storage, typename Allocator>
		class store_action<std::vector<Storage, Allocator>> final
		    : public action_base {
			std::vector<Storage, Allocator>* ptr;

		public:
			template <typename... Names>
			explicit store_action(std::vector<Storage, Allocator>* dst,
			                      Names&&... names)
			    : action_base(std::forward<Names>(names)...), ptr(dst) {
				action_base::multiple(true);
			}

			bool needs_arg() const override { return true; }
			using action::visit;
			void visit(parser& p, std::string const& arg) override {
				ptr->push_back(converter<Storage>::value(p, arg, argname(p)));
				visited(true);
			}
		};

		template <typename Storage,
		          typename Hash,
		          typename Eq,
		          typename Allocator>
		class store_action<std::unordered_set<Storage, Hash, Eq, Allocator>>
		    final : public action_base {
			std::unordered_set<Storage, Hash, Eq, Allocator>* ptr;

		public:
			template <typename... Names>
			explicit store_action(
			    std::unordered_set<Storage, Hash, Eq, Allocator>* dst,
			    Names&&... names)
			    : action_base(std::forward<Names>(names)...), ptr(dst) {
				action_base::multiple(true);
			}

			bool needs_arg() const override { return true; }
			using action::visit;
			void visit(parser& p, std::string const& arg) override {
				ptr->insert(converter<Storage>::value(p, arg, argname(p)));
				visited(true);
			}
		};

		template <typename Storage, typename Value>
		class set_value : public action_base {
			Storage* ptr;

		public:
			template <typename... Names>
			explicit set_value(Storage* dst, Names&&... names)
			    : action_base(std::forward<Names>(names)...), ptr(dst) {}

			bool needs_arg() const override { return false; }
			using action::visit;
			void visit(parser&) override {
				*ptr = Value::value;
				visited(true);
			}
		};

		namespace detail {
			template <typename Callable, bool WithParser, typename... Args>
			class custom_adapter_impl {
			public:
				custom_adapter_impl(Callable&& cb) : cb(std::move(cb)) {}
				void operator()(parser& p, Args... args) {
					cb(p, std::forward<Args>(args)...);
				}

			private:
				Callable cb;
			};

			template <typename Callable, typename... Args>
			class custom_adapter_impl<Callable, false, Args...> {
			public:
				custom_adapter_impl(Callable&& cb) : cb(std::move(cb)) {}
				void operator()(parser&, Args... args) {
					cb(std::forward<Args>(args)...);
				}

			private:
				Callable cb;
			};

			template <typename Callable, typename... Args>
			using custom_adapter = custom_adapter_impl<
			    Callable,
			    std::is_invocable_v<Callable, parser&, Args...>,
			    Args...>;

#if defined(HAS_STD_CONCEPTS)
			template <typename Callable, typename... Args>
			concept ActionHandler =
			    std::invocable<Callable, Args...> ||
			    (std::invocable<Callable, parser&, Args...> &&
			     !std::invocable<Callable, parser const&, Args...>);
#else
			template <typename Callable, typename... Args>
			constexpr bool is_action_handler_v =
			    std::is_invocable_v<Callable, Args...> ||
			    (std::is_invocable_v<Callable, parser&, Args...> &&
			     !std::is_invocable_v<Callable, parser const&, Args...>);
#endif
		}  // namespace detail

#if defined(HAS_STD_CONCEPTS)
		template <typename Callable>
		class custom_action;
#else
		template <typename Callable, typename Enable = void>
		class custom_action;
#endif

#if defined(HAS_STD_CONCEPTS)
		template <typename Callable>
		requires detail::ActionHandler<Callable> class custom_action<Callable>
#else
		template <typename Callable>
		class custom_action<
		    Callable,
		    std::enable_if_t<detail::is_action_handler_v<Callable>>>
#endif
		    : public action_base {
			detail::custom_adapter<Callable> cb;

		public:
			template <typename... Names>
			explicit custom_action(Callable&& cb, Names&&... names)
			    : action_base(std::forward<Names>(names)...)
			    , cb(std::move(cb)) {}

			bool needs_arg() const override { return false; }
			using action::visit;
			void visit(parser& p) override {
				cb(p);
				visited(true);
			}
		};

#if defined(HAS_STD_CONCEPTS)
		template <typename Callable>
		requires detail::ActionHandler<
		    Callable,
		    std::string const&> class custom_action<Callable>
#else
		template <typename Callable>
		class custom_action<
		    Callable,
		    std::enable_if_t<
		        detail::is_action_handler_v<Callable, std::string const&>>>
#endif
		    : public action_base {
			detail::custom_adapter<Callable, std::string const&> cb;

		public:
			template <typename... Names>
			explicit custom_action(Callable&& cb, Names&&... names)
			    : action_base(std::forward<Names>(names)...)
			    , cb(std::move(cb)) {}

			bool needs_arg() const override { return true; }
			using action::visit;
			void visit(parser& p, std::string const& s) override {
				cb(p, s);
				visited(true);
			}
		};
	}  // namespace actions
}  // namespace args
