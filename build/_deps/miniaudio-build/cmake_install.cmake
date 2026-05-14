# Install script for directory: /home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/miniaudio" TYPE FILE FILES "/home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-src/miniaudio.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/miniaudio/extras/nodes/ma_channel_combiner_node" TYPE FILE FILES "/home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-src/extras/nodes/ma_channel_combiner_node/ma_channel_combiner_node.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/miniaudio/extras/nodes/ma_channel_separator_node" TYPE FILE FILES "/home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-src/extras/nodes/ma_channel_separator_node/ma_channel_separator_node.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/miniaudio/extras/nodes/ma_ltrim_node" TYPE FILE FILES "/home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-src/extras/nodes/ma_ltrim_node/ma_ltrim_node.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/miniaudio/extras/nodes/ma_reverb_node" TYPE FILE FILES "/home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-src/extras/nodes/ma_reverb_node/ma_reverb_node.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/miniaudio/extras/nodes/ma_vocoder_node" TYPE FILE FILES "/home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-src/extras/nodes/ma_vocoder_node/ma_vocoder_node.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-build/miniaudio.pc")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-build/libminiaudio.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-build/libminiaudio_channel_combiner_node.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-build/libminiaudio_channel_separator_node.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-build/libminiaudio_ltrim_node.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-build/libminiaudio_reverb_node.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/ben/desktop/Personal/Projects/Ongoing Tic-Tac-Toe Optimized Bot/build/_deps/miniaudio-build/libminiaudio_vocoder_node.a")
endif()

