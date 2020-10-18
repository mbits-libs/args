cmake_minimum_required(VERSION 3.12)
project(args
  VERSION 0.12.1
  DESCRIPTION ""
  LANGUAGES CXX)

set(PROJECT_VERSION_STABILITY "") # or "-alpha", or "-beta", or "-rc.5"

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  message(STATUS "Libargs: Standalone")

  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup(TARGETS)

  set(LIBARG_TESTING_DEFAULT ON)
  set(LIBARG_INSTALL_DEFAULT ON)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  # It needs at least 17 to compile, but can use concepts to protect some interfaces.
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED OFF)

else()
  message(STATUS "Libargs: Subdir")
  set(LIBARG_TESTING_DEFAULT OFF)
  set(LIBARG_INSTALL_DEFAULT OFF)
endif()


set(LIBARGS_TESTING ${LIBARG_TESTING_DEFAULT} CACHE BOOL "Compile and/or run self-tests")
set(LIBARGS_INSTALL ${LIBARG_INSTALL_DEFAULT} CACHE BOOL "Install the library")
set(LIBARGS_SHARED OFF CACHE BOOL "Build an .so instead of the .a archive")

if (LIBARGS_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  message(STATUS "Python3_EXECUTABLE=${Python3_EXECUTABLE}")

  set(COVERALLS_PREFIX LIBARGS_)
  set(LIBARGS_COVERALLS_DIRS include/args src)
  include(tools/coveralls/Coveralls.cmake)
endif()

set(LIBARGS_WALL_FLAGS ON CACHE BOOL "Compile with -Wall/-W4 warning levels")

if (LIBARGS_WALL_FLAGS)
  if (MSVC)
    set(ADDITIONAL_WALL_FLAGS
        /permissive-
        /Zc:__cplusplus
        /W4
        /w14242
        /w14254
        /w14263
        /w14265
        /w14287
        /we4289
        /w14296
        /w14311
        /w14545
        /w14546
        /w14547
        /w14549
        /w14555
        /w14619
        /w14640
        /w14826
        /w14905
        /w14906
        /w14928
        /w14946)
  else()
    set(ADDITIONAL_WALL_FLAGS
        -Wall -Wextra
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
    )
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      list(APPEND ADDITIONAL_WALL_FLAGS -fcolor-diagnostics) # -Wlifetime
    else()
      list(APPEND ADDITIONAL_WALL_FLAGS
        -fdiagnostics-color
        -Wmisleading-indentation
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wuseless-cast
        )
    endif()
  endif()
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/version.in" "${CMAKE_CURRENT_BINARY_DIR}/include/args/version.hpp" @ONLY)

set(BUILD_SHARED_LIBS ${LIBARGS_SHARED})
add_library(args
  src/actions.cpp
  src/parser.cpp
  src/printer.cpp
  src/translator.cpp
  include/args/actions.hpp
  include/args/api.hpp
  include/args/parser.hpp
  include/args/printer.hpp
  include/args/translator.hpp
  "${CMAKE_CURRENT_BINARY_DIR}/include/args/version.hpp"
  )
target_compile_options(args PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_compile_definitions(args PRIVATE LIBARGS_EXPORTING)
target_compile_features(args PRIVATE cxx_std_17)
target_include_directories(args
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (LIBARGS_SHARED)
  target_compile_definitions(args PUBLIC LIBARGS_SHARED)
endif()
  
include(CheckCXXSourceCompiles)
function(check_charconv)
  check_cxx_source_compiles("#include <charconv>
int main() {}" HAS_CHARCONV_)
  set(HAS_CHARCONV ${HAS_CHARCONV_} PARENT)
endfunction()
check_cxx_source_compiles("#include <charconv>
int main() {}" HAS_CHARCONV)

if (NOT HAS_CHARCONV)
  message(FATAL_ERROR "The compiler has no access to <charconv>")
endif()

##################################################################
##  INSTALL
##################################################################

if (LIBARGS_INSTALL)
  install(TARGETS args)
  install(DIRECTORY include/args DESTINATION include)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/args/version.hpp" DESTINATION include/args)
endif()

##################################################################
##  TESTING
##################################################################

if (LIBARGS_TESTING)

add_executable(args-test tests/args-test.cpp)
target_compile_options(args-test PRIVATE ${ADDITIONAL_WALL_FLAGS})
set_target_properties(args-test
  PROPERTIES
    FOLDER tests
    )
target_link_libraries(args-test args)

add_test(
  NAME args.exit
  COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/args-test.py"
    "$<TARGET_FILE:args-test>"
    )
enable_testing()

endif()
