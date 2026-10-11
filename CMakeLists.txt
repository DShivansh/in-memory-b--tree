cmake_minimum_required(VERSION 3.16)

project(bplustree LANGUAGES CXX)

add_library(btree STATIC btree.cpp btree.hpp)
target_compile_features(btree PUBLIC cxx_std_17)
set_target_properties(btree PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(btree PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(btree_demo main.cpp)
target_link_libraries(btree_demo PRIVATE btree)
