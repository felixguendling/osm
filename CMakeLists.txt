cmake_minimum_required(VERSION 3.10)
project(osm)

include(cmake/pkg.cmake)


# --- LIBRARY ---
file(GLOB_RECURSE osm-files src/*.cc)
add_library(osm ${osm-files})
target_compile_features(osm PUBLIC cxx_std_23)
target_include_directories(osm SYSTEM PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(osm protozero cista utl geo zlibstatic Boost::fiber)


# --- TESTS ---
file(GLOB_RECURSE osm-test-files test/*.cc)
add_executable(osm-test ${osm-test-files})
target_link_libraries(osm-test gtest osm)
target_compile_definitions(osm-test PRIVATE TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(osm-test PRIVATE ${osm-compile-options})