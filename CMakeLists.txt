cmake_minimum_required(VERSION 3.28)
project(ClubSystem)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(main polished_main.cpp)
add_executable(tests tests.cpp utils.cpp catch_amalgamated.cpp)
