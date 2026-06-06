cmake_minimum_required(VERSION 3.16)
project(ConsoleChessProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(chess main.cpp)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_chess tests/test_chess.cpp)
    add_test(NAME chess_tests COMMAND test_chess)
endif()
