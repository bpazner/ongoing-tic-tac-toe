cmake_minimum_required(VERSION 3.16)
project(ott LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(miniaudio)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
list(FILTER SRC_FILES EXCLUDE REGEX ".*(scene|stb_image|miniaudio)\\.cpp$")

set(UI_SRC_FILES src/scene.cpp src/stb_image.cpp src/miniaudio.cpp)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

set(COMMON_INCLUDES
    ${PROJECT_SOURCE_DIR}/include
)

set(UI_INCLUDES
    ${COMMON_INCLUDES}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${stb_SOURCE_DIR}
    ${miniaudio_SOURCE_DIR}
)

#######################################################################
## MAIN
#######################################################################

add_executable(main EXCLUDE_FROM_ALL
    main.cpp
    ${SRC_FILES}
    ${UI_SRC_FILES}
    ${IMGUI_SOURCES}
)
target_include_directories(main PRIVATE ${UI_INCLUDES})
target_link_libraries(main PRIVATE glfw GL pthread dl)
target_compile_options(main PRIVATE -O3)

add_executable(main-debug EXCLUDE_FROM_ALL
    main.cpp
    ${SRC_FILES}
    ${UI_SRC_FILES}
    ${IMGUI_SOURCES}
)
target_include_directories(main-debug PRIVATE ${UI_INCLUDES})
target_link_libraries(main-debug PRIVATE glfw GL pthread dl)
target_compile_options(main-debug PRIVATE -O0 -g)

#######################################################################
## TARGETS
#######################################################################

add_custom_target(default ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Usage: make [release OR debug]"
)

add_custom_target(release
    DEPENDS main
)

add_custom_target(debug
    DEPENDS main-debug
)
