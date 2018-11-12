/*
 * Copyright (C) 2015 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace args {
	class parser;

	enum class lng {
		usage,
		def_meta,
		positionals,
		optionals,
		help_description,
		unrecognized,
		needs_param,
		requires,
		error_msg
	};

	struct base_translator {
		virtual ~base_translator() = default;
		virtual std::string operator()(lng id, std::string_view arg1 = { }, std::string_view arg2 = { }) const = 0;
	};

	class null_translator: public base_translator {
		static std::string s(std::string_view sv) {
			return { sv.data(), sv.length() };
		}
	public:
		std::string operator()(lng id, std::string_view arg1, std::string_view arg2) const override
		{
			switch (id) {
			case lng::usage:            return "usage: ";
			case lng::def_meta:		    return "ARG";
			case lng::positionals: 	    return "positional arguments";
			case lng::optionals:	    return "optional arguments";
			case lng::help_description: return "show this help message and exit";
			case lng::unrecognized:	    return "unrecognized argument: " + s(arg1);
			case lng::needs_param:	    return "argument " + s(arg1) + ": expected one argument";
			case lng::requires:		    return "argument " + s(arg1) + " is required";
			case lng::error_msg:	    return s(arg1) + ": error: " + s(arg2);
			}
			return "<unrecognized string>";
		}
	};

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
			std::string meta(const base_translator& _) const override {
				return meta_.empty() ? _(lng::def_meta) : meta_;
			}
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
				std::is_invocable<Callable, parser&, Args...>::value,
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

	namespace detail {
		bool is_terminal(FILE* out) noexcept;
		int terminal_width(FILE* out) noexcept;

		template <typename It>
		inline It split(It cur, It end, size_t width) noexcept
		{
			if (size_t(end - cur) <= width)
				return end;

			auto c_end = cur + width;
			auto it = cur;
			while (true) {
				auto prev = it;
				while (it != c_end && *it == ' ') ++it;
				while (it != c_end && *it != ' ') ++it;
				if (it == c_end) {
					if (prev == cur || *c_end == ' ')
						return c_end;
					return prev;
				}
			}
		}

		template <typename It>
		inline It skip_ws(It cur, It end) noexcept
		{
			while (cur != end && *cur == ' ') ++cur;
			return cur;
		}
	}

	struct chunk {
		std::string title;
		std::vector<std::pair<std::string, std::string>> items;
	};

	using fmt_list = std::vector<chunk>;

	struct file_printer {
		file_printer(FILE* out) : out(out) { }
		void print(const char* cur, size_t len) noexcept
		{
			if (len)
				fwrite(cur, 1, len, out);
		}
		void putc(char c) noexcept { fputc(c, out); }
		size_t width() const noexcept
		{
			if (detail::is_terminal(out))
				return detail::terminal_width(out);
			return 0;
		}
	private:
		FILE* out;
	};

	template <typename output>
	struct printer_base_impl : output {
		using output::output;
		inline void format_paragraph(const std::string& text, size_t indent, size_t width)
		{
			if (width < 2)
				width = text.length();
			else
				--width;

			if (indent >= width)
				indent = 0;

			auto cur = text.begin();
			auto end = text.end();
			auto chunk = detail::split(cur, end, width);

			output::print(&*cur, chunk - cur);
			output::putc('\n');

			cur = detail::skip_ws(chunk, end);
			if (cur == end)
				return;

			std::string pre(indent, ' ');
			width -= (int)indent;

			while (cur != end) {
				chunk = detail::split(cur, end, width);
				output::print(pre.c_str(), pre.length());
				output::print(&*cur, chunk - cur);
				output::putc('\n');
				cur = detail::skip_ws(chunk, end);
			}
		}
		inline void format_list(const fmt_list& info, size_t width)
		{
			size_t len = 0;
			for (auto& chunk : info) {
				for (auto& [opt, descr] : chunk.items) {
					if (len < opt.length())
						len = opt.length();
				}
			}

			if (width < 20) { // magic treshold
				for (auto& chunk : info) {
					output::putc('\n');
					output::print(chunk.title.c_str(), chunk.title.length());
					output::print(":\n", 2);
					for (auto& [opt, descr] : chunk.items) {
						output::putc(' ');
						output::print(opt.c_str(), opt.length());
						for (size_t i = 0, max = len - opt.length() + 1; i < max; ++i)
							output::putc(' ');
						output::print(descr.c_str(), descr.length());
						output::putc('\n');
					}
				}

				return;
			}

			auto proposed = width / 3;
			len += 2;
			if (len > proposed)
				len = proposed;
			len -= 2;

			for (auto& chunk : info) {
				output::putc('\n');
				format_paragraph(chunk.title + ":", 0, width);
				for (auto&[opt, descr] : chunk.items) {
					auto prefix = (len < opt.length() ? opt.length() : len) + 2;

					std::string sum;
					sum.reserve(prefix + descr.length());
					sum.push_back(' ');
					sum.append(opt);
					// prefix - (initial space + the value for the first column)
					sum.append(prefix - 1 - opt.length(), ' ');
					sum.append(descr);
					format_paragraph(sum, prefix, width);
				}
			}
		}
	};

	template <typename output>
	struct printer_base : printer_base_impl<output> {
		using printer_base_impl<output>::printer_base_impl;
	};

	template <>
	struct printer_base<file_printer> : printer_base_impl<file_printer> {
		using printer_base_impl<file_printer>::printer_base_impl;
		using printer_base_impl<file_printer>::format_paragraph;
		using printer_base_impl<file_printer>::format_list;
		inline void format_paragraph(const std::string& text, size_t indent,
			std::optional<size_t> maybe_width = {})
		{
			format_paragraph(text, indent, maybe_width.value_or(width()));
		}
		inline void format_list(const fmt_list& info,
			std::optional<size_t> maybe_width = {})
		{
			format_list(info, maybe_width.value_or(width()));
		}
	};

	using printer = printer_base<file_printer>;

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
