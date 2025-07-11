set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
  GIT_SHALLOW TRUE)

if(NOT CMAKE_HOST_SYSTEM_NAME MATCHES "Windows" AND MSVC)
  set(gtest_disable_pthreads
      OFF ON
      CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  subprocess
  GIT_REPOSITORY https://github.com/shediao/subprocess.hpp
  GIT_TAG v0.0.3
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(subprocess)

function(add_environment_test test_name test_files)
  add_executable(${test_name} ${test_files})
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_executable(${test_name}_ansi ${test_files})
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
     OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
     OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    if(NOT MSVC)
      target_compile_options(${test_name} PRIVATE -Wall -Wextra -Werror)
      if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        target_compile_options(${test_name}_ansi PRIVATE -Wall -Wextra -Werror)
      endif()
    endif()
  endif()
  if(MSVC)
    target_compile_options(${test_name} PRIVATE /utf-8 /EHsc /W4)

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
      target_compile_options(${test_name}_ansi PRIVATE /utf-8 /EHsc /W4)
    endif()
    target_compile_definitions(${test_name} PRIVATE UNICODE _UNICODE)
  endif()

  target_link_libraries(
    ${test_name} PRIVATE environment::environment GTest::gtest
                         GTest::gtest_main subprocess::subprocess)
  add_test(NAME ${test_name} COMMAND ${test_name})
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(
      ${test_name}_ansi PRIVATE environment::environment GTest::gtest
                                GTest::gtest_main subprocess::subprocess)
    add_test(NAME ${test_name}_ansi COMMAND ${test_name})
  endif()
endfunction()

file(GLOB test_files "*.cc")

foreach(test_file ${test_files})
  get_filename_component(test_name ${test_file} NAME_WE)
  add_environment_test(${test_name} ${test_file})
endforeach()

add_environment_test(all_test "${test_files}")
