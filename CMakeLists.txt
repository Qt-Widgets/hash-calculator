cmake_minimum_required(VERSION 3.5)

project(hashcalculator)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(DEPLOYQT_EXECUTABLE NAMES windeployqt macdeployqt HINTS "${_qt_bin_dir}")

set(HEADERS hashcalculator.h widget.h)
set(SOURCES hashcalculator.cpp widget.cpp main.cpp)
set(UIS widget.ui)

if(WIN32)
    list(APPEND SOURCES hashcalculator.rc)
endif()

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES} ${UIS})

if(WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${DEPLOYQT_EXECUTABLE} --force --no-translations --no-system-d3d-compiler --no-compiler-runtime --no-angle --no-opengl-sw "$<TARGET_FILE:${PROJECT_NAME}>")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets)
