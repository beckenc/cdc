cmake_minimum_required(VERSION 3.0.0)
project(cpc VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(src lib)
add_subdirectory(src)
add_subdirectory(lib)

include(CTest)
enable_testing()
add_subdirectory(test)
