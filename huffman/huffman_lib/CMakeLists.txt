project(huffman-lib)

set(SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/huffman.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/binary_io/binary_reader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/binary_io/binary_writer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/huffman_convert_tree/convert_tree.cpp)

set(HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/huffman.h
        ${CMAKE_CURRENT_SOURCE_DIR}/binary_io/binary_reader.h
        ${CMAKE_CURRENT_SOURCE_DIR}/binary_io/binary_writer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/huffman_convert_tree/convert_tree.h)

add_library(huffman-lib ${SOURCES} ${HEADERS})
target_include_directories(huffman-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
