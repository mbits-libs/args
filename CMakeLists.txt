if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  cmake_minimum_required (VERSION 3.10)
  project (args CXX)
  set(LIBARG_TESTING_DEFAULT ON)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
else()
  message(STATUS "Subdir")
  set(LIBARG_TESTING_DEFAULT OFF)
endif()
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "${CMAKE_SOURCE_DIR}")


set(LIBARGS_TESTING ${LIBARG_TESTING_DEFAULT} CACHE BOOL "Compile and/or run self-tests")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (LIBARGS_TESTING)
  set(COVERALLS_DIRS include/args src)
  include(tools/coveralls/Coveralls.cmake)
endif()

add_library(args STATIC
  src/argparser.cpp
  include/args/argparser.hpp
  )
target_include_directories(args
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

##################################################################
##  TESTING
##################################################################

if (LIBARGS_TESTING)

add_executable(args-test tests/args-test.cpp)
set_target_properties(args-test PROPERTIES FOLDER tests)
target_link_libraries(args-test args)

add_test(NAME args.exit COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tests/args-test.py" "$<TARGET_FILE:args-test>")
enable_testing()

endif()
