cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(noisetoy VERSION 1.0)

# find source files:
file(GLOB_RECURSE noisetoy_src CONFIGURE_DEPENDS "src/*.cpp")
add_executable(${PROJECT_NAME} ${noisetoy_src})

# find include directories and libraries:
find_package(GLFW3 3.3 REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# add dependencies:
include_directories("src/")
target_link_libraries(${PROJECT_NAME} glfw glad::glad imgui::imgui)

# set working directory:
set_property(TARGET ${PROJECT_NAME} PROPERTY WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/assets/")
if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/assets/")
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DPI_AWARE "PerMonitor") # set dpi awareness (only works on windows)
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
endif()