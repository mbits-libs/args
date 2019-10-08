// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <args/printer.hpp>

#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#define _isatty(FD) isatty(FD)
#define _fileno(OBJ) fileno(OBJ)
#endif

bool args::detail::is_terminal(FILE* out) noexcept {
	return _isatty(_fileno(out)) != 0;
}

size_t args::detail::terminal_width(FILE* out) noexcept {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO buffer = {};
	auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(out)));
	if (handle == INVALID_HANDLE_VALUE ||
	    !GetConsoleScreenBufferInfo(handle, &buffer))
		return 0;
	return buffer.dwSize.X < 0 ? 0u : static_cast<size_t>(buffer.dwSize.X);
#else
	winsize w;
	while (ioctl(fileno(out), TIOCGWINSZ, &w) == -1) {
		if (errno != EINTR) return 0;
	}
	return w.ws_col;
#endif
}
