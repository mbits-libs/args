// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#if defined(_WIN32)
#define LIBARGS_EXPORT __declspec(dllexport)
#define LIBARGS_IMPORT __declspec(dllimport)
#else
#define LIBARGS_EXPORT
#define LIBARGS_IMPORT
#endif

#if defined(LIBARGS_SHARED)
#if defined(LIBARGS_EXPORTING)
#define LIBARGS_API LIBARGS_EXPORT
#else
#define LIBARGS_API LIBARGS_IMPORT
#endif
#else
#define LIBARGS_API
#endif
