cmake_minimum_required(VERSION 3.21)
project(TuxTI83 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# App-layer QML resources (Style singleton, Main.qml, all UI components).
# The legacy graph_ui/qml/Main.qml resource block was removed 2026-04-07
# alongside the file's deletion at the end of Phase A.
qt_add_resources(tux_ti83 "app_qml"
    PREFIX "/App"
    BASE "app/qml"
    FILES
        "app/qml/qmldir"
        "app/qml/Style.qml"
        "app/qml/Main.qml"
        "app/qml/components/CalcKey.qml"
        "app/qml/components/Display.qml"
        "app/qml/components/SoftKeyRow.qml"
        "app/qml/components/HistoryPane.qml"
        "app/qml/components/WindowPopup.qml"
        "app/qml/components/MatrixPopup.qml"
        "app/qml/components/FunctionSelector.qml"
        "app/qml/components/GraphCanvas.qml"
        "app/qml/components/MathMenuPopup.qml"
)
