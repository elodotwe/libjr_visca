cmake_minimum_required(VERSION 3.20)

project(jr_visca)

add_library(jr_visca STATIC jr_visca.c jr_visca.h)
target_include_directories(jr_visca PUBLIC .)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)