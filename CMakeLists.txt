# project settings
cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
project(vlux LANGUAGES CXX)
set(LIB_NAME ${PROJECT_NAME}-lib)

# cpp defaults
set(CMAKE_C_COMPILER /usr/bin/gcc CACHE PATH "")
set(CMAKE_CXX_COMPILER /usr/bin/g++ CACHE PATH "")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")

# set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# project settings and add targets
add_executable(${PROJECT_NAME})
add_library(${LIB_NAME})

# cpp settings
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_features(${LIB_NAME} PRIVATE cxx_std_23)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-missing-field-initializers)
target_compile_options(${LIB_NAME} PRIVATE -Wno-missing-field-initializers)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw vulkan dl pthread X11 Xxf86vm Xrandr Xi)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIB_NAME})

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

# add modules
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED True)

# dependencies
include(cmake/dependencies.cmake)

# subdirectory
set(SRC_DIR "./src")
add_subdirectory(${SRC_DIR})