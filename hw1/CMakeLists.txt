cmake_minimum_required(VERSION 3.16.3)
project(skeleton_smash)

set(CMAKE_CXX_STANDARD 11)

add_executable(skeleton_smash smash.cpp Commands.cpp signals.cpp)