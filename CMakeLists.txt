cmake_minimum_required(VERSION 3.20)

project(jr_visca)
enable_testing()

add_library(jr_visca STATIC jr_visca.c jr_visca.h)
target_include_directories(jr_visca PUBLIC .)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

add_executable(jr_visca_tester jr_visca_tester.c)
target_link_libraries(jr_visca_tester jr_visca)
add_test(NAME jr_visca_tests COMMAND jr_visca_tester)