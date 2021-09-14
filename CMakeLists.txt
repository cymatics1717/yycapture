cmake_minimum_required(VERSION 3.14)

project(yycapture VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        minimal/obscapture.hpp
        minimal/obscapture.cpp
        obscap/obs_wrapper.hpp
        obscap/capture_manager.cpp
        obscap/capture_manager.hpp
        obscap/capture_source_wrapper.hpp
        obscap/capture_source_wrapper.cpp
        obscap/display_wrapper.cpp
        obscap/display_wrapper.hpp
        obscap/obs_core_wrapper.cpp
        obscap/obs_core_wrapper.hpp
        obscap/scene_wrapper.cpp
        obscap/scene_wrapper.hpp
        obscap/utils.cpp
        obscap/utils.hpp

        qml.qrc
)

set(OBS_BUILD D:/obs/build-obs-studio-Desktop_Qt_5_15_2_MSVC2019_64bit-Debug)
set(OBS_SOURCE D:/obs/obs-studio)

include_directories(obscap)
include_directories(minimal)
include_directories(${OBS_SOURCE})
include_directories(${OBSPATH}/libobs)
add_library(libobs SHARED IMPORTED)
set_target_properties(libobs PROPERTIES IMPORTED_LOCATION ${OBS_BUILD}/libobs/Debug/obs.dll)
set_target_properties(libobs PROPERTIES IMPORTED_IMPLIB ${OBS_BUILD}/libobs/Debug/obs.lib)
set_target_properties(libobs PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${OBS_SOURCE}/libobs)

add_executable(yycapture
  ${PROJECT_SOURCES}
)

target_compile_definitions(yycapture
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(yycapture
  PRIVATE Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Quick
  libobs
  )

set_target_properties(yycapture PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_import_qml_plugins(yycapture)
    qt_finalize_executable(yycapture)
endif()
