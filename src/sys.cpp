// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <args/sys.hpp>
#include <cstdlib>
#include <exception>
#include <utility>

namespace args {
	namespace {
		[[noreturn]] void std_exit(int code) {
			std::exit(code);
		}

		exit_function global__ = std_exit;
	}  // namespace

	exit_function set_exit(exit_function new_fun) {
		std::swap(global__, new_fun);
		return new_fun;
	}

	[[noreturn]] void exit(int code) {
		global__(code);
		std::terminate();
	}
}  // namespace args
