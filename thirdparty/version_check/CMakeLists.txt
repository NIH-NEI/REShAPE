
cmake_minimum_required(VERSION 3.10.0)
cmake_policy(SET CMP0020 NEW)
cmake_policy(SET CMP0076 NEW)
project("VERSION_CHECK")

### VTK
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

message(STATUS "VTK_DIR: ${VTK_DIR}")
set(VTK_VER "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
message(STATUS "VTK_VER: ${VTK_VER}")
message(STATUS "VTK_MODULES_DIR: ${VTK_MODULES_DIR}")

### Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

message(STATUS "Qt5_DIR: ${Qt5_DIR}")
message(STATUS "Qt5Core_VERSION: ${Qt5Core_VERSION}")

### ITK
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

message(STATUS "ITK_DIR: ${ITK_DIR}")
set(ITK_VER "${ITK_VERSION_MAJOR}.${ITK_VERSION_MINOR}")
message(STATUS "ITK_VER: ${ITK_VER}")
