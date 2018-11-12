// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace args {
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
}
