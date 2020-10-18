# args

[![Travis (.org)](https://img.shields.io/travis/mbits-libs/args)](https://travis-ci.org/mbits-libs/args)
[![Coveralls github](https://img.shields.io/coveralls/github/mbits-libs/args)](https://coveralls.io/github/mbits-libs/args?branch=master)

Small open-source library for program argument parser, inspired by Python's `argparse`, depending only on the standard library, with C++17 as minimum requirement.

It automatically builds and prints out help info for program arguments, can react to missing or unrecognized arguments and has support for arguments with and without values. It can also support placing long lists of arguments into an [@answer-file](#parseruse_answer_file).

The argument values can be stored in:
- `std::string`s,
- things, which can be constructed from strings (excluding `std::string_view`) &mdash; for example `std::filesystem::path`,
- all things `int` (uses `std::is_integral`, but excludes `bool`),
- enums, with little help from library user (uses `std::is_enum` and needs `args::enum_traits` provided), as well as
- `std::optional`, `std::vector` and `std::unordered_set` of the things on this list.

## Config

Type of all CMake variables blow is `BOOL`.

|Project variable|Comment|
|----------------|-------|
|`LIBARGS_TESTING`|Compile and/or run self-tests. _Adds `test` target (`RUN_TESTS` on MSVC), which runs automatic tests._ |
|`LIBARGS_INSTALL`|Install the library. _Adds `install` target (`INSTALL` on MSVC), which installs all distributable files inside `CMAKE_INSTALL_PREFIX` directory. On MSVC it operates on `Release` configuration._ |
|`LIBARGS_SHARED`|Build an .so instead of the .a archive. _Setting this variable is still experimental. Since this is pre-1.0 code, installing a shared library system-wide is not advised in the first place, in addition there is little experience in using this library as a shared entity._ |
|`LIBARGS_COVERALLS`|Turn on coveralls support. _Ads `coveralls` target, which runs tests and gathers coverage results in the Coveralls.io compatible JSON file. This variable is only available, if `LIBARGS_TESTING` is true. On Ubuntu, it works only with `gcov`, `llvm`-based solution is not available. On MSVC it operates on `Debug` configuration and requires [OpenCppCoverage](https://github.com/OpenCppCoverage/OpenCppCoverage) installed and on `%PATH%`._|

## Building

Conan is used for proper `MD`/`MDd`/`MT`/`MTd` setup. This project does not require any dependencies otherwise. The `<Config>` used in recipes below is either `Debug` or `Release`.

### Ubuntu

```sh
mkdir build && cd build
conan install .. -s build_type=<Config>
cmake .. \
  -G Ninja \
  -DLIBARGS_TESTING=OFF \
  -DLIBARGS_INSTALL=OFF \
  -DCMAKE_BUILD_TYPE=<Config>
ninja
```

### Windows

The `<RT>` below is either `MD` or `MT` for `Release`, or `MDd` or `MTd` for `Debug`. If `LIBARGS_SHARED` is set to `ON`, then `MD`/`MDd` is strongly advised.

```sh
mkdir build && cd build
conan install .. -s build_type=<Config> -s compiler.runtime=<RT>
cmake .. -G Ninja -DLIBARGS_TESTING=OFF -DLIBARGS_INSTALL=OFF
cmake --build . --target ALL_BUILD -- ^
  /nologo /v:m /p:Configuration=<Config>
```

## Testing

### Ubuntu

```sh
mkdir build && cd build
conan install .. -s build_type=<Config>
cmake .. \
  -G Ninja \
  -DLIBARGS_TESTING=ON \
  -DLIBARGS_INSTALL=OFF \
  -DCMAKE_BUILD_TYPE=<Config>
ninja && ninja test
```

### Windows

```sh
mkdir build && cd build
conan install .. -s build_type=<Config> -s compiler.runtime=<RT>
cmake .. -G Ninja -DLIBARGS_TESTING=ON -DLIBARGS_INSTALL=OFF
cmake --build . --target RUN_TESTS -- ^
  /nologo /v:m /p:Configuration=<Config>
```

## Installing

### Ubuntu

```sh
mkdir build && cd build
conan install .. -s build_type=Release
cmake .. \
  -G Ninja \
  -DLIBARGS_TESTING=OFF \
  -DLIBARGS_INSTALL=ON \
  -DCMAKE_BUILD_TYPE=Release
ninja && sudo ninja install
```

### Windows

```sh
mkdir build && cd build
conan install .. -s build_type=Release -s compiler.runtime=MD
cmake .. -G Ninja -DLIBARGS_TESTING=OFF -DLIBARGS_INSTALL=ON
cmake --build . --target RUN_TESTS -- ^
  /nologo /v:m /p:Configuration=Release
```

## Example

To mimic Python example:

```cxx
#include <args/parser.hpp>
#include <numeric>
#include <vector>

unsigned accumulate(std::vector<unsigned> const& integers, auto pred) {
    return std::accumulate(integers.begin(), integers.end(), 0u, pred);
}

int main(int argc, char* argv[]) {
    std::vector<unsigned> integers{};
    bool is_sum{false};

    args::null_translator tr{};
    args::parser parser{"Process some integers.",
        args::from_main(argc, argv), &tr};
    parser.set<std::true_type>(is_sum, "sum")
        .opt()
        .help("sum the integers (default: find the max)");
    parser.arg(integers)
        .meta("N")
        .help("an integer for the accumulator");

    parser.parse();

    auto const result =
        is_sum ? accumulate(integers, [](auto a, auto b) { return a + b; })
               : accumulate(integers,
                            [](auto a, auto b) { return std::max(a, b); });
    printf("%u\n", result);
}
```
Assuming the code above is compiled to `prog`, it can be used to provide help information:

```
$ ./prog -h
usage: prog [-h] [--sum] N [N ...]

Process some integers.

positional arguments:
 N          an integer for the accumulator

optional arguments:
 -h, --help show this help message and exit
 --sum      sum the integers (default: find the max)
```

When run with arguments, it prints either the sum, or max, or complains about the command line:

```
$ ./prog 1 2 3 4
4
```
```
$ ./prog 1 2 3 4 --sum
10
```
```
$ ./prog a b c
usage: prog [-h] [--sum] N [N ...]
prog: error: argument N: expected a number
```
```
$ ./prog --sum
usage: prog [-h] [--sum] N [N ...]
prog: error: argument N is required
```

## Argument formats

The arguments with values can have those values either separated by a space or can have it glued to the name of the argument; in case of long names, glued values need to be separated from names by a `"="`.

|Width|Example|Separated|Glued|Standalone|
|-----|-------|--------------|-----|-------------|
|Long|`"arg"`|`--arg value`|`--arg=value`|`--arg`|
|Short|`"c"`, `"z"`, `"f"`|`-f value`|`-fvalue`|`-c -z`|

Short arguments can also be glued together, except that any argument with a value will take the rest of argument for itself. This makes next lines equivalent:

```plain
-c -z -f value
-c -z -fvalue
-czf value
-czfvalue
```

# Classes

## args::arglist

Extremely simple class, which would be `std::span<char const*>`, if this was targeting C++20 alone.

## args::args_view

A structure holding program name and argument list.

### args::from_main

```cxx
args_view from_main(int, char**);
args_view from_main(arglist const&);
```

Free function constructing `args_view` objects. Using the first element, locates the filename of the program and (on Windows) removes extension. Shifts left remaining arguments. Uses program filename and shifted arguments to create the `args_view`.

## args::base_translator, args::null_translator

Translator is a class with an `operator()`, which takes a message id and produces appropriate translation.

### null_translator::operator()

```cxx
std::string operator()(lng id,
                       std::string_view arg1 = {},
                       std::string_view arg2 = {}) const override;
```

Produces built in translations for given string ids:

|lng|result|
|---|------|
|`usage`|`"usage: "`|
|`def_meta`|`"ARG"`|
|`positionals`|`"positional arguments"`|
|`optionals`|`"optional arguments"`|
|`help_description`|`"show this help message and exit"`|
|`unrecognized`|`"unrecognized argument: $arg1"`|
|`needs_param`|`"argument $arg1: expected one argument"`|
|`needs_no_param`|`"argument $arg1: value was not expected"`|
|`needs_number`|`"argument $arg1: expected a number"`|
|`needed_number_exceeded`|`"argument $arg1: number outside of expected bounds"`|
|`needed_enum_unknown`|`"argument $arg1: value $arg2 is not recognized"`|
|`needed_enum_known_values`|`"known values for $arg1: $arg2"`|
|`required`|`"argument $arg1 is required"`|
|`error_msg`|`"$arg1: error: $arg2"`|
|`file_not_found`|`"cannot open $arg1"`|

## args::enum_traits&lt;Enum&gt;

Extension point for the enum arguments, helping convert strings from command line to particular enum value. Best way to provide specialization of this template, is to add series of the macros between enum definition and its usage in the parser:

1. `ENUM_TRAITS_BEGIN(enum-name)` \
    provides all the machinery for `args::enum_traits<enum-name>` specialization.
2. One or more of either
   - `ENUM_TRAITS_NAME(enum-value)` or
   - `ENUM_TRAITS_NAME_EX(enum-value, "string-to-use")`

    which provide the string-to-enum mapping; one-argument version uses `enum-name` as scope for `enum-value`, while two-argument version is much lower level and the value is left not scoped.
3. `ENUM_TRAITS_END(enum-name)` \
    finishes all the work started by `ENUM_TRAITS_BEGIN(enum-name)`.

The macros open namespace `args`, so they need to be called from global namespace for this tool to work.

### Example

```cpp
#include <args/parser.hpp>
#include <iostream>

enum my_app {
    enum class opt { unspecified, always, never, automatic };
}

// my_app::opt::unspecified is missing
ENUM_TRAITS_BEGIN(my_app::opt)
    ENUM_TRAITS_NAME(never)
    ENUM_TRAITS_NAME(always)
    ENUM_TRAITS_NAME_EX(my_app::opt::automatic, "auto")
ENUM_TRAITS_END(my_app::opt)

int main(int argc, char* argv[]) {
    my_app::opt option{my_app::opt::unspecified};

    args::null_translator tr{};
    args::parser parser{"Use an enum class.",
        args::from_main(argc, argv), &tr};
    parser.arg(option, "option").meta("WHEN").opt();

    parser.parse();

    std::cout << option == my_app::opt::unspecified
        ? "Option unspecified.\n"
        : "Option read.\n";
}
```

Assuming once again the code above is compiled to `prog`, it will behave nicely, as long as `--option` will be either `always`, `never` or `auto`:

```
$ ./prog
Option unspecified.
```
```
$ ./prog --option always
Option read.
```
```
$ ./prog --option never
Option read.
```
But once we stop using names listed with the macros, the parser will exit with an error:
```
$ ./prog --option unspecified
usage: prog [-h] [--option WHEN]
prog: error: argument --option: value unspecified is not recognized
known values for --option: never, always, auto
```
```
$ ./prog --option waffles
usage: prog [-h] [--option WHEN]
prog: error: argument --option: value waffles is not recognized
known values for --option: never, always, auto
```

## args::actions::builder

Returned from `parser::set<Value>`, `parser::add` and `parser::custom`, allows to tweak the argument in addition to what was provided in said `parser` methods.

```cxx
unsigned verbosity{3};
parser.custom([&]{ ++verbosity; }, "v")
      .opt()
      .multi()
      .help("increases debug log level");
```

### builder::meta

```cxx
builder& builder::meta(std::string_view name);
```

Provides a name for the value in arguments with value for both `parser::help` and `parser::short_help`. Defaults to value translated from `lng::def_meta`.

### builder::help

```cxx
builder& builder::help(std::string_view dscr);
```

Provides help message for the `parser::help` output.

### builder::multi

```cxx
builder& builder::multi(bool value = true);
```

Provides the cardinality hint for `parser::short_help`; otherwise not used. Defaults to `true` for `std::vector` and `std::unordered_set` and to `false` otherwise.

### builder::req, builder::opt

```cxx
builder& builder::req(bool value = true);
builder& builder::opt(bool value = true); // same as "req(!value)"
```

Provide the cardinality hint for `parser::short_help` and aid in `parser::parse()` aftermath. The underlying attribute, "required", defaults to `false` for `std::optional` and to `true` otherwise.

## args::parser

Main class of the library.

### parser::parser

```cxx
parser(std::string description, args_view const& args, base_translator const* tr);
parser(std::string description, std::string_view progname, arglist const& args, base_translator const* tr);
parser(std::string description, arglist const& args, base_translator const* tr);
```

Constructs a parser with given description, arguments and translator. Second version packs `progname` and `args` into `args_view` and calls first version. Third version uses `from_main` helper and calls first version.

### parser::set\<Value\>

```cxx
template <typename Value, typename Storage, typename... Names>
actions::builder set(Storage& dst, Names&&... names);
```

Creates an argument, which does not need a value and assumes `Value` is `std::integral_constant`-like class by setting the `dst` with `Value::value`, when triggered. Useful with boolean flags. In such scenario, the variable behind would be set to one value and the opposite type would be selected for the `set`:

```cxx
bool foo{true};
bool bar{false};

parser.set<std::false_type>(foo, "no-foo");
parser.set<std::true_type>(bar, "bar");
```

`Names`, if given, will be used for argument names. One-letter names will create single-dash arguments, longer names will create double-dash arguments. Empty `names` list will result in positional argument.

### parser::arg

```cxx
template <typename Storage, typename... Names>
actions::builder arg(Storage& dst, Names&&... names);
```

Creates an argument with a value. If the `Storage` type is `std::optional`, the argument will not be required. If the `Storage` type is either `std::vector` or `std::unordered_set`, the created argument will add new items to the container.

`Names`, if given, will be used for argument names. One-letter names will create single-dash arguments, longer names will create double-dash arguments. Empty `names` list will result in positional argument.

### parser::custom

```cxx
template <typename Callable, typename... Names>
actions::builder custom(Callable cb, Names&&... names);
```

Creates an argument with custom callback. If the callback does not take any arguments, or takes a single `args::parser&` argument, then this is an argument without a value. If the callback takes either string type, or `args::parser&` followed by a string type, then this is an argument with a value. String type is one of `std::string`, `std::string_view`, `std::string const&` or `std::string_view const&`.

`Names`, if given, will be used for argument names. One-letter names will create single-dash arguments, longer names will create double-dash arguments. Empty `names` list will result in positional argument.

```cxx
parser.custom([]() { ... }, "foo", "f"); // used as: "--foo" or "-f"
parser.custom([](std::string_view arg) { ... }, "Z"); // used as: "-Z qux"
parser.custom([](args::parser& p) { ... }, "A"); // used as: "-A"
parser.custom([](args::parser& p, std::string_view arg) { ... }, "bar"); // used as: "--bar baz"
```

### parser::parse

```cxx
arglist parse(unknown_action on_unknown = exclusive_parser,
              std::optional<size_t> maybe_width = {});
```

Parses the arguments. The `on_unknown` instructs the parser, how to react on an argument without matching action. The default `parser::exclusive_parser` will print an error and terminate program; `parser::allow_subcommands` will stop processing and will return remaining arguments.

The `maybe_width` is a hint for terminal width. If missing, will try to get the current width of a terminal. If the program is not attached to terminal, will print the messages without any formatting.

### parser::use_answer_file

```cxx
void use_answer_file(char marker = '@');
bool uses_answer_file() const noexcept;
char answer_file_marker() const noexcept;
```

Turns on/off support for answer files in argument list. By default, `marker` is set to 0, which turns the support off.

```c++
assert(!parser.uses_answer_file());
parser.use_answer_file();
assert(parser.uses_answer_file());
```

When turned on, will use the marker character to recognize the name of the answer file and use further arguments from there. For instance, if a program decides to support answer files and file named `options` contains this text:

```
-vv
-v
--arg1=value1
--arg2
value2
--arg3=value3
--arg4
```

Then those calls are equivalent:

```
$ ./prog -vvv --arg1 value1 --arg2 value2 --arg3=value3 --arg4
$ ./prog @options
```

### parser::provide_help

```cxx
void provide_help(bool value);
bool provides_help() const noexcept;
```

Turns off/on the built in support for `"-h"` and `"--help"` arguments.

### parser::short_help

```cxx
void short_help(FILE* out = stdout,
                bool for_error = false,
                std::optional<size_t> maybe_width = {}) const;
```

Prints "usage" line, with program name and possible arguments and their optionality/cardinality.

The `maybe_width` is a hint for terminal width. If missing, will try to get the current width of a terminal. If the program is not attached to terminal, will print the messages without any formatting.

### parser::help

```cxx
[[noreturn]] void help(std::optional<size_t> maybe_width = {}) const;
```

Prints `short_help()` and follows with description and arguments, finally calls `std::exit(0)`.

The `maybe_width` is a hint for terminal width. If missing, will try to get the current width of a terminal. If the program is not attached to terminal, will print the messages without any formatting.

### parser::error

```cxx
[[noreturn]] void error(std::string const& msg,
                        std::optional<size_t> maybe_width = {}) const;
```

Prints `short_help()` and follows with error message, finally calls `std::exit` with non-zero value.

The `maybe_width` is a hint for terminal width. If missing, will try to get the current width of a terminal. If the program is not attached to terminal, will print the messages without any formatting.
