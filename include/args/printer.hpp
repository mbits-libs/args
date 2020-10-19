// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <args/api.hpp>
#include <optional>
#include <string>
#include <vector>

namespace args {
	namespace detail {
		LIBARGS_API bool is_terminal(FILE* out) noexcept;
		LIBARGS_API size_t terminal_width(FILE* out) noexcept;

		template <typename It>
		inline It split(It cur, It end, size_t width) noexcept {
			if (size_t(end - cur) <= width) return end;

			auto c_end = cur + static_cast<ptrdiff_t>(width);
			auto it = cur;
			while (true) {
				auto prev = it;
				while (it != c_end && *it == ' ')
					++it;
				while (it != c_end && *it != ' ')
					++it;
				if (it == c_end) {
					if (prev == cur || *c_end == ' ') return c_end;
					return prev;
				}
			}
		}

		template <typename It>
		inline It skip_ws(It cur, It end) noexcept {
			while (cur != end && *cur == ' ')
				++cur;
			return cur;
		}
	}  // namespace detail

	struct chunk {
		std::string title;
		std::vector<std::pair<std::string, std::string>> items;
	};

	using fmt_list = std::vector<chunk>;

	struct file_printer {
		file_printer(FILE* out) : out(out) {}
		void print(char const* cur, size_t len) noexcept {
			if (len) fwrite(cur, 1, len, out);
		}
		void putc(char c) noexcept { fputc(c, out); }
		size_t width() const noexcept {
			if (detail::is_terminal(out)) return detail::terminal_width(out);
			return 0;
		}

	private:
		FILE* out;
	};

	template <typename output>
	struct printer_base_impl : output {
		using output::output;

		inline bool leading(char c) noexcept {
			auto const uc = static_cast<unsigned char>(c);
			static constexpr auto leading_mask = 0xC0u;
			static constexpr auto leading_value = 0x80u;
			return (uc & leading_mask) == leading_value;
		}

		inline size_t utf8len(std::string_view view) {
			size_t count{};

			for (auto c : view)
				count += !leading(c);

			return count;
		}

		inline void format_paragraph(std::string const& text,
		                             size_t indent,
		                             size_t width) {
			if (width < 2)
				width = text.length();
			else
				--width;

			if (indent >= width) indent = 0;

			auto cur = text.begin();
			auto end = text.end();
			auto chunk = detail::split(cur, end, width);

			output::print(&*cur, static_cast<size_t>(chunk - cur));
			output::putc('\n');

			cur = detail::skip_ws(chunk, end);
			if (cur == end) return;

			std::string pre(indent, ' ');
			width -= indent;

			while (cur != end) {
				chunk = detail::split(cur, end, width);
				output::print(pre.c_str(), pre.length());
				output::print(&*cur, static_cast<size_t>(chunk - cur));
				output::putc('\n');
				cur = detail::skip_ws(chunk, end);
			}
		}

		inline void format_list(fmt_list const& info, size_t width) {
			size_t len = 0;
			for (auto& chunk : info) {
				for (auto& [opt, descr] : chunk.items) {
					len = std::max(len, utf8len(opt));
				}
			}

			if (width < 20) {  // magic treshold
				for (auto& chunk : info) {
					output::putc('\n');
					output::print(chunk.title.c_str(), chunk.title.length());
					output::print(":\n", 2);
					for (auto& [opt, descr] : chunk.items) {
						output::putc(' ');
						output::print(opt.c_str(), opt.length());
						for (size_t i = 0, max = len - utf8len(opt) + 1;
						     i < max; ++i)
							output::putc(' ');
						output::print(descr.c_str(), descr.length());
						output::putc('\n');
					}
				}

				return;
			}

			auto proposed = width / 3;
			len += 2;
			if (len > proposed) len = proposed;
			len -= 2;

			for (auto& chunk : info) {
				output::putc('\n');
				format_paragraph(chunk.title + ":", 0, width);
				for (auto& [opt, descr] : chunk.items) {
					auto const spaces = len - utf8len(opt) + 1;

					std::string sum;
					sum.reserve(opt.length() + spaces + descr.length() + 1);
					sum.push_back(' ');
					sum.append(opt);
					sum.append(spaces, ' ');
					sum.append(descr);
					format_paragraph(sum, len + 2, width);
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
		inline void format_paragraph(std::string const& text,
		                             size_t indent,
		                             std::optional<size_t> maybe_width = {}) {
			format_paragraph(text, indent, maybe_width.value_or(width()));
		}
		inline void format_list(fmt_list const& info,
		                        std::optional<size_t> maybe_width = {}) {
			format_list(info, maybe_width.value_or(width()));
		}
	};

	using printer = printer_base<file_printer>;
}  // namespace args
