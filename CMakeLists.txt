cmake_minimum_required(VERSION 3.21)
project(TuxTI83 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Gui Qml Quick)

add_library(core_math core_math/src/core_math.cpp)
target_include_directories(core_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core_math/include)

add_library(graph_ui
    graph_ui/src/ui_controller.cpp
    graph_ui/include/ui_controller.hpp
)
target_include_directories(graph_ui PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/graph_ui/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/core_math/include
)
target_link_libraries(graph_ui PUBLIC core_math Qt6::Gui Qt6::Qml Qt6::Quick)

add_executable(tux_ti83 app/main.cpp)
target_link_libraries(tux_ti83 PRIVATE graph_ui Qt6::Gui Qt6::Qml Qt6::Quick)

# Fixed Resource Mapping: Ensure Main.qml is at the root of the QRC
qt_add_resources(tux_ti83 "qml"
    PREFIX "/"
    FILES
        "graph_ui/qml/Main.qml"
)
