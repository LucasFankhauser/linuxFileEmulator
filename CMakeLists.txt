cmake_minimum_required(VERSION 3.31)
project(linuxFileEmulator C)

set(CMAKE_C_STANDARD 11)

add_executable(linuxFileEmulator main.c
        filesystem.c
        filesystem.h)
