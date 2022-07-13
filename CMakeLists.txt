cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(coro LANGUAGES CXX)

if (POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

if (MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.29.30129)  # 16.11
    set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "-std:c++latest")
    set(CMAKE_CXX20_EXTENSION_COMPILE_OPTION "-std:c++latest")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# fmt
include(FetchContent)
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)
# google benchmark
set(BENCHMARK_ENABLE_TESTING off)
FetchContent_Declare(googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark
  GIT_TAG master
)
FetchContent_MakeAvailable(
  fmt
  googlebenchmark
)


# boost context (external)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS context)


# warnings as errors (directory-wide)
# need move these compile options after fetched contents to avoid affect them
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()


add_executable(test test/loop.cpp)
target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test PRIVATE fmt::fmt)

add_executable(bench bench/bench.cpp)
# remove target compile option
# get_target_property(bench_options bench COMPILE_OPTIONS)
# message(STATUS "${bench_options}")
# if (MSVC)
#   list(REMOVE_ITEM bench_options "/WX")
# else()
#   list(REMOVE_ITEM bench_options "-Werror")
# endif()
# message(STATUS "${bench_options}")
# set_property(TARGET bench PROPERTY COMPILE_OPTIONS ${bench_options})

target_include_directories(bench PRIVATE
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(bench PRIVATE 
  fmt::fmt
  benchmark::benchmark
  Boost::context
)

add_executable(switch_coro_test test/switch_coro_test.cpp)
