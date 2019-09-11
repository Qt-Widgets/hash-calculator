cmake_minimum_required(VERSION 3.5)

project(hashcalculator LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(HEADERS hashcalculator.h widget.h)
set(SOURCES hashcalculator.cpp widget.cpp main.cpp)
set(UIS widget.ui)

add_executable(hashcalculator ${HEADERS} ${SOURCES} ${UIS})

target_link_libraries(hashcalculator PRIVATE Qt5::Widgets)
