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

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.21
)

set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/_deps)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
    FetchContent_Populate(miniaudio)
endif()

add_executable(plane_war
    src/main.cpp
    src/game.cpp
    src/audio.cpp
)

target_include_directories(plane_war PRIVATE src ${miniaudio_SOURCE_DIR})

target_link_libraries(plane_war PRIVATE
    ftxui::dom
    ftxui::screen
    ftxui::component
)

# Static link libgcc/libstdc++ on MinGW (reduces DLL dependencies at runtime)
if(MINGW)
    target_link_libraries(plane_war PRIVATE -static-libgcc -static-libstdc++)
endif()

if(WIN32)
    target_link_libraries(plane_war PRIVATE winmm)
elseif(UNIX)
    target_link_libraries(plane_war PRIVATE pthread dl m)
endif()

# Copy assets and data to build directory after build
add_custom_command(TARGET plane_war POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:plane_war>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:plane_war>/data"
    COMMENT "Copying assets and data to build directory...")
