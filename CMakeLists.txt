# project settings
cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
project(vlux LANGUAGES CXX)
set(LIB_NAME ${PROJECT_NAME}-lib)

# macos
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MACOS TRUE)
endif()

# cpp defaults
if(${MACOS})
  set(CMAKE_C_COMPILER
      /opt/homebrew/Cellar/gcc/13.2.0/bin/gcc-13
      CACHE PATH "")
  set(CMAKE_CXX_COMPILER
      /opt/homebrew/Cellar/gcc/13.2.0/bin/g++-13
      CACHE PATH "")
else()
  set(CMAKE_C_COMPILER
      /usr/bin/gcc
      CACHE PATH "")
  set(CMAKE_CXX_COMPILER
      /usr/bin/g++
      CACHE PATH "")
endif()

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
if(${MACOS})
  include_directories($ENV{HOME}/VulkanSDK/1.3.280.1/macOS/include/)
  link_libraries($ENV{HOME}/VulkanSDK/1.3.280.1/macOS/lib/libvulkan.dylib)
  include_directories(/opt/homebrew/Cellar/glm/1.0.1/include/)
  include_directories(/opt/homebrew/Cellar/glfw/3.4/include/)
  link_libraries(/opt/homebrew/Cellar/glfw/3.4/lib/libglfw.dylib)
endif()

# cpp settings
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_features(${LIB_NAME} PRIVATE cxx_std_23)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-missing-field-initializers)
target_compile_options(${LIB_NAME} PRIVATE -Wno-missing-field-initializers)
if(${MACOS})
  target_link_libraries(${PROJECT_NAME} PUBLIC dl pthread)
else()
  target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC glfw
           vulkan
           dl
           pthread
           X11
           Xxf86vm
           Xrandr
           Xi)
endif()
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
