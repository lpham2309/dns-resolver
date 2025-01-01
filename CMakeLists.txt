cmake_minimum_required(VERSION 3.29)
project(dns_resolver)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(dns_resolver
    main.cpp
    main.h)
