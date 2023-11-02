cmake_minimum_required(VERSION 3.24)
project(minilua_learn C)

set(CMAKE_C_STANDARD 11)

add_executable(minilua_learn minilua.c minilua.h mllib.c)
