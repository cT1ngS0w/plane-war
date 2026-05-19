cmake_minimum_required(VERSION 3.14)
project(plane-war VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG        v6.1.9
)

set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/_deps)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_executable(plane_war
    src/main.cpp
    src/game.cpp
)

target_include_directories(plane_war PRIVATE src)

target_link_libraries(plane_war PRIVATE
    ftxui::dom
    ftxui::screen
    ftxui::component
)
