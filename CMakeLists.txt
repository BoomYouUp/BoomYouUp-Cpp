cmake_minimum_required(VERSION 3.24)
project(BoomYouUp)

set(CMAKE_CXX_STANDARD 23)

add_executable(BoomYouUp main.cpp)

target_link_libraries(BoomYouUp -static)
