cmake_minimum_required(VERSION 3.21)
project(TuxTI83 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(Qt6 REQUIRED COMPONENTS Gui Qml Quick)

add_library(core_math core_math/src/core_math.cpp)
target_include_directories(core_math PUBLIC core_math/include)

add_library(graph_ui graph_ui/src/ui_controller.cpp)
target_include_directories(graph_ui PUBLIC graph_ui/include core_math/include)
target_link_libraries(graph_ui PUBLIC core_math Qt6::Gui Qt6::Qml Qt6::Quick)

add_executable(tux_ti83 app/main.cpp)
target_link_libraries(tux_ti83 PRIVATE graph_ui Qt6::Gui Qt6::Qml Qt6::Quick)
qt_add_resources(tux_ti83 "qml" PREFIX "/" FILES graph_ui/qml/Main.qml)
