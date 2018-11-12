// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <string>
#include <type_traits>
#include <vector>

namespace args {
	class parser;
	struct base_translator;

	namespace actions {
		struct action {
			virtual ~action();
			virtual bool required() const = 0;
			virtual void required(bool value) = 0;
			virtual bool multiple() const = 0;
			virtual void multiple(bool value) = 0;
			virtual bool needs_arg() const = 0;
			virtual void visit(parser&) = 0;
			virtual void visit(parser&, const std::string& /*arg*/) = 0;
			virtual bool visited() const = 0;
			virtual void meta(const std::string& s) = 0;
			virtual std::string meta(const base_translator&) const = 0;
			virtual void help(const std::string& s) = 0;
			virtual const std::string& help() const = 0;
			virtual bool is(const std::string_view& name) const = 0;
			virtual bool is(char name) const = 0;
			virtual const std::vector<std::string>& names() const = 0;

			void append_short_help(const base_translator& _, std::string& s) const;
			std::string help_name(const base_translator& _) const;

		protected:
			action();
			action(const action&) = delete;
			action(action&&);
			action& operator=(const action&) = delete;
			action& operator=(action&&);
		};

		class builder {
			friend class ::args::parser;

			action* ptr;
			builder(action* ptr) : ptr(ptr) {}
		public:
			builder(const builder&) = delete;
			builder(builder&&) = default;
			builder& operator=(const builder&) = delete;
			builder& operator=(builder&&) = default;

			builder& meta(const std::string& name) {
				ptr->meta(name);
				return *this;
			}
			builder& help(const std::string& dscr) {
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
			static void pack(std::vector<std::string>& dst, Name&& name, Names&&... names)
			{
				dst.emplace_back(std::forward<Name>(name));
				pack(dst, std::forward<Names>(names)...);
			}

		protected:
			template <typename... Names>
			action_base(Names&&... argnames)
			{
				pack(names_, std::forward<Names>(argnames)...);
			}

			void visited(bool val) { visited_ = val; }
		public:
			void required(bool value) override { required_ = value; }
			bool required() const override { return required_; }
			void multiple(bool value) override { multiple_ = value; }
			bool multiple() const override { return multiple_; }

			void visit(parser&) override { visited_ = true; }
			void visit(parser&, const std::string& /*arg*/) override { visited_ = true; }
			bool visited() const override { return visited_; }
			void meta(const std::string& s) override { meta_ = s; }
			std::string meta(const base_translator& _) const override;
			void help(const std::string& s) override { help_ = s; }
			const std::string& help() const override { return help_; }

			bool is(const std::string_view& name) const override
			{
				for (auto& argname : names_) {
					if (argname.length() > 1 && argname == name)
						return true;
				}

				return false;
			}

			bool is(char name) const override
			{
				for (auto& argname : names_) {
					if (argname.length() == 1 && argname[0] == name)
						return true;
				}

				return false;
			}

			const std::vector<std::string>& names() const override
			{
				return names_;
			}
		};

		template <typename T>
		class store_action final : public action_base {
			T* ptr;
		public:
			template <typename... Names>
			explicit store_action(T* dst, Names&&... names) : action_base(std::forward<Names>(names)...), ptr(dst) {}

			bool needs_arg() const override { return true; }
			void visit(parser&, const std::string& arg) override
			{
				*ptr = arg;
				visited(true);
			}
		};

		template <typename T, typename Allocator>
		class store_action<std::vector<T, Allocator>> final : public action_base {
			std::vector<T, Allocator>* ptr;
		public:
			template <typename... Names>
			explicit store_action(std::vector<T, Allocator>* dst, Names&&... names)
				: action_base(std::forward<Names>(names)...), ptr(dst)
			{
				action_base::multiple(true);
			}

			bool needs_arg() const override { return true; }
			void visit(parser&, const std::string& arg) override
			{
				ptr->push_back(arg);
				visited(true);
			}
		};

		template <typename T, typename Value>
		class set_value : public action_base {
			T* ptr;
		public:
			template <typename... Names>
			explicit set_value(T* dst, Names&&... names) : action_base(std::forward<Names>(names)...), ptr(dst) {}

			bool needs_arg() const override { return false; }
			void visit(parser&) override
			{
				*ptr = Value::value;
				visited(true);
			}
		};


		namespace detail {
			template <typename Callable, bool Forward, typename... Args>
			class custom_adapter_impl {
			public:
				custom_adapter_impl(Callable cb) : cb(std::move(cb))
				{
				}
				void operator()(parser& p, Args&&... args)
				{
					cb(p, std::forward<Args>(args)...);
				}
			private:
				Callable cb;
			};

			template <typename Callable, typename... Args>
			class custom_adapter_impl<Callable, false, Args...> {
			public:
				custom_adapter_impl(Callable cb) : cb(std::move(cb))
				{
				}
				void operator()(parser&, Args&&... args)
				{
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

			template <typename Callable, typename ... Args>
			constexpr bool is_compatible_with_v =
				std::is_invocable_v<Callable, Args...> ||
				std::is_invocable_v<Callable, parser&, Args...>;
		}

		template <typename Callable, typename Enable = void> class custom_action;

		template <typename Callable>
		class custom_action<Callable, std::enable_if_t<detail::is_compatible_with_v<Callable>>> : public action_base {
			detail::custom_adapter<Callable> cb;
		public:
			template <typename... Names>
			explicit custom_action(Callable cb, Names&&... names) : action_base(std::forward<Names>(names)...), cb(std::move(cb)) {}

			bool needs_arg() const override { return false; }
			void visit(parser& p) override
			{
				cb(p);
				visited(true);
			}
		};

		template <typename Callable>
		class custom_action<Callable, std::enable_if_t<detail::is_compatible_with_v<Callable, const std::string&>>> : public action_base {
			detail::custom_adapter<Callable> cb;
		public:
			template <typename... Names>
			explicit custom_action(Callable cb, Names&&... names) : action_base(std::forward<Names>(names)...), cb(std::move(cb)) {}

			bool needs_arg() const override { return true; }
			void visit(parser& p, const std::string& s) override
			{
				cb(p, s);
				visited(true);
			}
		};
	}
}
