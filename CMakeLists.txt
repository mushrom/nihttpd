cmake_minimum_required(VERSION 2.8)
project("httpd thing")

include_directories( "include" )

SET( HTTPD_CXXFLAGS "-O2 -std=c++14" )
SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${HTTPD_CXXFLAGS}" )

add_executable(nihttpd src/main.cpp)
