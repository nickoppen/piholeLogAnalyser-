cmake_minimum_required(VERSION 3.15)
set(CMAKE_SYSTEM_NAME Generic)


#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

#cross-compilation toolchain configuration
set(CMAKE_CROSSCOMPILING true)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_LIBRARY_ARCHITECTURE aarch64-linux-gnu)
set(CMAKE_C_COMPILER ${CMAKE_LIBRARY_ARCHITECTURE}-gcc)
set(CMAKE_CXX_COMPILER ${CMAKE_LIBRARY_ARCHITECTURE}-g++)
set(CMAKE_LINKER ${CMAKE_LIBRARY_ARCHITECTURE}-ld)

project(piholeLogAnalyser CXX)

set(LIBAARCH64 /lib/aarch64-linux-gnu)  #
set(LIBUSR /usr/lib)
set(INCLUDEUSR /usr/include)

set(GROK++ ${PROJECT_SOURCE_DIR}/Grok++)
set(ANALYSER ${PROJECT_SOURCE_DIR}/piholeLogAnalyser++)

set(CMAKE_INSTALL_RPATH ${CMAKE_BUILD_RPATH})

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${GROK++}
    ${ANALYSER}
    ${INCLUDEUSR}
)


add_executable(${PROJECT_NAME} ${ANALYSER}/main.cpp ${ANALYSER_SOURCES}) #${GROK_SOURCES})

add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME} ${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} -L"${GROK++}" grok++)
target_link_libraries(${PROJECT_NAME} -L"${LIBAARCH64}" mariadbcpp)
