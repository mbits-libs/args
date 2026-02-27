// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

namespace args {
	using exit_function = void (*)(int);
	exit_function set_exit(exit_function);
	[[noreturn]] void exit(int code);
}  // namespace args
