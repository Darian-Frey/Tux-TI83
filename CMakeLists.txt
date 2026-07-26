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
    graph_ui/src/crash_logger.cpp
    graph_ui/include/crash_logger.hpp
)
target_include_directories(graph_ui PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/graph_ui/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/core_math/include
)
target_link_libraries(graph_ui PUBLIC core_math Qt6::Gui Qt6::Qml Qt6::Quick)

add_executable(tux_ti83 app/main.cpp)
target_link_libraries(tux_ti83 PRIVATE graph_ui Qt6::Gui Qt6::Qml Qt6::Quick)

# Headless one-shot CLI — evaluate a single expression and exit.
# Usage: tux_ti83_cli "2+2". Wrap expressions with `!` in single quotes
# to avoid bash history expansion.
add_executable(tux_ti83_cli cli/cli_main.cpp)
target_link_libraries(tux_ti83_cli PRIVATE graph_ui Qt6::Core Qt6::Gui)

# Interactive REPL — prompt-per-line calculator with Ans recall
# between lines. Reads stdin, so bash history expansion doesn't apply.
add_executable(tux_ti83_repl cli/repl_main.cpp)
target_link_libraries(tux_ti83_repl PRIVATE graph_ui Qt6::Core Qt6::Gui)

# Regression test binary — drives UIController programmatically and
# asserts on the displayed result. Run with `./build/tux_ti83_tests`.
add_executable(tux_ti83_tests tests/test_math.cpp)
target_link_libraries(tux_ti83_tests PRIVATE graph_ui Qt6::Core Qt6::Gui)
enable_testing()
add_test(NAME math_regression COMMAND tux_ti83_tests)

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
        "app/qml/components/ListPopup.qml"
        "app/qml/components/StatResultsPopup.qml"
        "app/qml/components/RegMenuPopup.qml"
        "app/qml/components/StatPlotPopup.qml"
        "app/qml/components/MEMPopup.qml"
        "app/qml/components/FunctionSelector.qml"
        "app/qml/components/GraphCanvas.qml"
        "app/qml/components/MathMenuPopup.qml"
        "app/qml/components/MODEPopup.qml"
        "app/qml/components/LogicMenuPopup.qml"
        "app/qml/components/CatalogPopup.qml"
        "app/qml/components/ZoomPopup.qml"
        "app/qml/components/TableView.qml"
        "app/qml/components/TblSetPopup.qml"
)
