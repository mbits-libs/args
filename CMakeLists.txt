if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  cmake_minimum_required (VERSION 3.10)
  project (args CXX)
  set(LIBARG_TESTING_DEFAULT ON)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
else()
  message(STATUS "Libargs: Subdir")
  set(LIBARG_TESTING_DEFAULT OFF)
endif()


set(LIBARGS_TESTING ${LIBARG_TESTING_DEFAULT} CACHE BOOL "Compile and/or run self-tests")

if (LIBARGS_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  message(STATUS "Python3_EXECUTABLE=${Python3_EXECUTABLE}")

  set(COVERALLS_DIRS include/args src)
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

add_library(args STATIC
  src/actions.cpp
  src/parser.cpp
  src/printer.cpp
  src/translator.cpp
  include/args/actions.hpp
  include/args/parser.hpp
  include/args/printer.hpp
  include/args/translator.hpp
  )
target_compile_options(args PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_include_directories(args
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

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
