# =============================================================================
#  VitalSigns Monitor
#  Desktop vital-signs monitor (C++20, Qt 6 / QML) for a simulated bedside
#  device. Synthetic data only - this is NOT a medical device.
# =============================================================================
cmake_minimum_required(VERSION 3.25)

project(VitalSignsMonitor
    VERSION 0.2.0
    DESCRIPTION "Desktop vital-signs monitor (C++20, Qt 6/QML) for a simulated bedside device"
    HOMEPAGE_URL "https://github.com/mohsenmazaheri/VitalSignsMonitor"
    LANGUAGES CXX)

# --- Language standard --------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Put every executable (app, simulator, tests) in one folder so that they share
# the same runtime DLLs on Windows.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# QML modules are generated under <build>/qml/<URI path> so that qmllint and
# Qt Creator find them (each module folder must end in its URI path).
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml")

# --- Build options ------------------------------------------------------------
option(VSM_BUILD_TESTS          "Build the unit tests (GoogleTest)" ON)
option(VSM_BUILD_SIMULATOR      "Build the synthetic device simulator" ON)
option(VSM_WARNINGS_AS_ERRORS   "Treat compiler warnings as errors" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)

# --- Qt -----------------------------------------------------------------------
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Multimedia Network SerialPort Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.8)

# --- Targets ------------------------------------------------------------------
add_subdirectory(src)

if(VSM_BUILD_SIMULATOR)
    add_subdirectory(tools/simulator)
endif()

if(VSM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
