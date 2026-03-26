cmake_minimum_required(VERSION 3.16)
project(TriggerScalars VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt MOC/RCC are handled automatically
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(WITH_ZEROMQ "Build with ZeroMQ support" ON)

# ---------------------------------------------------------------------------
# Qt
# ---------------------------------------------------------------------------
# On Homebrew macOS, qt@5 is keg-only: the CMake configs in the shared
# /opt/homebrew/lib/cmake/Qt5* have a broken install prefix (they point at
# /opt/homebrew/ instead of the keg), causing a missing mkspecs error.
# Auto-detect the keg prefix via `brew --prefix qt@5` and prepend it to
# CMAKE_PREFIX_PATH so CMake uses the correct keg-local configs.
# This is a no-op on Linux or any system without Homebrew.
option(USE_QT6 "Build against Qt6 instead of Qt5" OFF)

if(APPLE AND NOT USE_QT6 AND NOT Qt5_DIR)
    find_program(_brew brew)
    if(_brew)
        execute_process(
            COMMAND ${_brew} --prefix qt@5
            OUTPUT_VARIABLE _brew_qt5
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
        if(_brew_qt5 AND EXISTS "${_brew_qt5}")
            message(STATUS "Homebrew qt@5 keg: ${_brew_qt5}")
            list(PREPEND CMAKE_PREFIX_PATH "${_brew_qt5}")
        endif()
    endif()
endif()

if(USE_QT6)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)
    set(QT_VERSION_MAJOR 5)
endif()

# ---------------------------------------------------------------------------
# yaml-cpp
# ---------------------------------------------------------------------------
find_package(yaml-cpp REQUIRED)

# ---------------------------------------------------------------------------
# ZeroMQ (optional)
# ---------------------------------------------------------------------------
if(WITH_ZEROMQ)
    # Try modern CMake config first (provided by cppzmq >= 4.3)
    find_package(cppzmq QUIET)
    if(NOT cppzmq_FOUND)
        # Fall back to pkg-config for distro packages
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(ZMQ REQUIRED libzmq)
        # Create an imported target to unify link syntax below
        add_library(cppzmq INTERFACE IMPORTED)
        target_include_directories(cppzmq INTERFACE ${ZMQ_INCLUDE_DIRS})
        target_link_libraries(cppzmq INTERFACE ${ZMQ_LINK_LIBRARIES})
    endif()
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/config/Config.cpp
    src/network/UdpReceiver.cpp
    src/gui/ScalarWidget.cpp
    src/gui/ScalarBank.cpp
    src/gui/MainWindow.cpp
)

if(WITH_ZEROMQ)
    list(APPEND SOURCES src/network/ZmqReceiver.cpp)
endif()

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
add_executable(trigger-scalers ${SOURCES})

target_include_directories(trigger-scalers PRIVATE src)

target_link_libraries(trigger-scalers PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    yaml-cpp::yaml-cpp
)

if(WITH_ZEROMQ)
    target_link_libraries(trigger-scalers PRIVATE cppzmq)
    target_compile_definitions(trigger-scalers PRIVATE WITH_ZEROMQ)
endif()

# ---------------------------------------------------------------------------
# Test sender tools
# ---------------------------------------------------------------------------
option(BUILD_TOOLS "Build UDP and ZeroMQ sender test tools" ON)

if(BUILD_TOOLS)
    # udp-sender: uses Qt Network for QUdpSocket
    add_executable(udp-sender tools/udp_sender.cpp)
    target_include_directories(udp-sender PRIVATE src tools)
    target_link_libraries(udp-sender PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network)

    # zmq-sender: plain C++ + ZMQ only (no Qt dependency)
    if(WITH_ZEROMQ)
        add_executable(zmq-sender tools/zmq_sender.cpp)
        target_include_directories(zmq-sender PRIVATE src tools)
        target_link_libraries(zmq-sender PRIVATE cppzmq)
    endif()
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS trigger-scalers DESTINATION bin)
install(FILES config/scalars.yaml DESTINATION share/trigger-scalers)

if(BUILD_TOOLS)
    install(TARGETS udp-sender DESTINATION bin)
    if(WITH_ZEROMQ)
        install(TARGETS zmq-sender DESTINATION bin)
    endif()
endif()
