cmake_minimum_required(VERSION 3.15)
project(ppr_exercise_2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp")
set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES *.h *.cpp)

add_executable(ppr_exercise_2 ${SOURCES} main.h)
