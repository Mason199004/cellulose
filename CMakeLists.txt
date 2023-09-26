cmake_minimum_required(VERSION 3.26)
project(cellulose C)

set(CMAKE_C_STANDARD 23)

add_library(cellulose cellulose.c
		cellulose.h)
