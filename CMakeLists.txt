cmake_minimum_required(VERSION 3.15)
project(
  environment
  VERSION "0.1"
  LANGUAGES CXX)

set("PROJECT_DESCRIPTION" "A header-only lightweight environment for c++20")
set("PROJECT_HOMEPAGE_URL" "https://github.com/shediao/environment.hpp")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  option(ENVIRONMENT_BUILD_TESTS "Set to ON to build tests" ON)
else()
  option(ENVIRONMENT_BUILD_TESTS "Set to ON to build tests" OFF)
endif()

add_library(environment INTERFACE)

target_include_directories(
  environment INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                       $<INSTALL_INTERFACE:include>)

target_compile_features(environment INTERFACE cxx_std_20)
add_library(environment::environment ALIAS environment)

if(ENVIRONMENT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

install(
  TARGETS environment
  EXPORT environment-targets
  INCLUDES
  DESTINATION include)

install(
  DIRECTORY include/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.hpp")

install(
  EXPORT environment-targets
  FILE environment-targets.cmake
  NAMESPACE environment::
  DESTINATION lib/cmake/environment)
