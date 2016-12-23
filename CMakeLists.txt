cmake_minimum_required (VERSION 2.6)

project (mutexpp)

file(GLOB APP_SRC ./src/*.cpp)

get_filename_component(APP_HEADERS_PATH ./include ABSOLUTE)

include_directories(mutexpp AFTER ${APP_HEADERS_PATH})

add_executable(mutexpp ${APP_SRC})

set_target_properties(mutexpp PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD c++0x)
set_target_properties(mutexpp PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY libc++)
