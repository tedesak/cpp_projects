cmake_minimum_required(VERSION 3.21)
project(huffman-unit-tests)

set(CMAKE_CXX_STANDARD 20)

find_package(GTest REQUIRED)

add_executable(tests tests.cpp
        ../huffman_lib/utils/constants.h
        ../huffman_lib/huffman.h
        ../huffman_lib/huffman.cpp
        ../huffman_lib/binary_io/binary_reader.cpp
        ../huffman_lib/binary_io/binary_reader.h
        ../huffman_lib/binary_io/binary_writer.cpp
        ../huffman_lib/binary_io/binary_writer.h
        ../huffman_lib/huffman_convert_tree/convert_tree.cpp
        ../huffman_lib/huffman_convert_tree/convert_tree.h)

target_link_libraries(tests GTest::gtest GTest::gtest_main)

target_compile_definitions(tests PRIVATE "ROOT_DIRECTORY=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
