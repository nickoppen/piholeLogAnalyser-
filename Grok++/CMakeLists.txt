
cmake_minimum_required(VERSION 3.15)


project(grok++ CXX)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
add_library(${PROJECT_NAME} grok.cpp) #grok.h grokException.h grokResult.h grokNamedSubExpression.h)

#Shared lib ending in .so
set(LIB_NAME "${CMAKE_SHARED_LIBRARY_PREFIX}${PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")

#Static lib ending in .a
#set(LIB_NAME "libgrok++.a")

add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${LIB_NAME} ${PROJECT_SOURCE_DIR})
