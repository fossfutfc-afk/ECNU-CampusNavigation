cmake_minimum_required(VERSION 3.34)
project(CampusNavigation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(
        CampusNavigation
        src/main.cpp
        src/GraphElements.cpp
        src/LGraph.cpp
        src/GraphAlgorithm.cpp
        src/CsvIO.cpp
        src/CommandProcessor.cpp
)

add_executable(
        testall
        test/testall.cpp
)

file(COPY ${CMAKE_SOURCE_DIR}/data/places.csv ${CMAKE_SOURCE_DIR}/data/roads.csv DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
