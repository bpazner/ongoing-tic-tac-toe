cmake_minimum_required(VERSION 3.16)
project(OTT LANGUAGES CXX)

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
## LINUX
#######################################################################

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_executable(OngoingTicTacToe
        app/main.cpp
        ${SRC_FILES}
        ${UI_SRC_FILES}
        ${IMGUI_SOURCES}
    )
    target_include_directories(OngoingTicTacToe PRIVATE ${UI_INCLUDES})
    target_link_libraries(OngoingTicTacToe PRIVATE glfw GL pthread dl)
    target_compile_options(OngoingTicTacToe PRIVATE -O3)
    target_link_options(OngoingTicTacToe PRIVATE -s)

    add_executable(OngoingTicTacToe-debug EXCLUDE_FROM_ALL
        app/main.cpp
        ${SRC_FILES}
        ${UI_SRC_FILES}
        ${IMGUI_SOURCES}
    )
    target_include_directories(OngoingTicTacToe-debug PRIVATE ${UI_INCLUDES})
    target_link_libraries(OngoingTicTacToe-debug PRIVATE glfw GL pthread dl)
    target_compile_options(OngoingTicTacToe-debug PRIVATE -O0 -g)

    add_executable(MultiplayerServer
        app/multiplayer_server.cpp
    )
    target_include_directories(MultiplayerServer PRIVATE ${COMMON_INCLUDES})
    target_link_libraries(MultiplayerServer PRIVATE pthread)
    target_compile_options(MultiplayerServer PRIVATE -O3)

    add_executable(MultiplayerServer-debug EXCLUDE_FROM_ALL
        app/multiplayer_server.cpp
    )
    target_include_directories(MultiplayerServer-debug PRIVATE ${COMMON_INCLUDES})
    target_link_libraries(MultiplayerServer-debug PRIVATE pthread)
    target_compile_options(MultiplayerServer-debug PRIVATE -O0 -g)

    add_custom_target(debug
        DEPENDS OngoingTicTacToe-debug MultiplayerServer-debug
    )
endif()

#######################################################################
## WINDOWS RELEASE (MinGW cross-compile)
#######################################################################

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(GLFW_WIN_DIR ${CMAKE_BINARY_DIR}/glfw-win)
    set(GLFW_WIN_ZIP ${CMAKE_BINARY_DIR}/glfw-win.zip)
    if(NOT EXISTS ${GLFW_WIN_DIR}/include)
        file(DOWNLOAD
            "https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip"
            ${GLFW_WIN_ZIP}
            SHOW_PROGRESS
        )
        file(ARCHIVE_EXTRACT INPUT ${GLFW_WIN_ZIP} DESTINATION ${CMAKE_BINARY_DIR}/glfw-win-extract)
        file(RENAME ${CMAKE_BINARY_DIR}/glfw-win-extract/glfw-3.4.bin.WIN64 ${GLFW_WIN_DIR})
    endif()

    set(GLFW_WIN_INCLUDE ${GLFW_WIN_DIR}/include)
    set(GLFW_WIN_LIB ${GLFW_WIN_DIR}/lib-mingw-w64/libglfw3.a)

    add_executable(OngoingTicTacToe WIN32
        app/main.cpp
        ${SRC_FILES}
        ${UI_SRC_FILES}
        ${IMGUI_SOURCES}
    )
    target_include_directories(OngoingTicTacToe PRIVATE ${UI_INCLUDES} ${GLFW_WIN_INCLUDE})
    target_link_libraries(OngoingTicTacToe PRIVATE
        ${GLFW_WIN_LIB}
        opengl32 gdi32 shell32 ole32
    )
    target_compile_options(OngoingTicTacToe PRIVATE -O3)
    target_link_options(OngoingTicTacToe PRIVATE -static -static-libgcc -static-libstdc++)
endif()