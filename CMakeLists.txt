cmake_minimum_required(VERSION 3.17.3 FATAL_ERROR)

project(Dyna-Kinematics LANGUAGES C CXX)

include_directories("${CMAKE_SOURCE_DIR}/inc"
                    "${CMAKE_SOURCE_DIR}/dependencies/win/inc")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

link_directories("${CMAKE_SOURCE_DIR}/dependencies/win/lib")

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui)

set(project_ui
    ui/rigid_body_simulator.ui)

set(project_headers
    inc/finite_state_machine.h
    inc/game.h
    inc/menu_state.h
    inc/renderer_2D.h
    inc/resource_manager.h
    inc/rigid_body_2D.h
    inc/rigid_body_simulator.h
    inc/shader.h
    inc/shader_loader.h
    inc/state.h
    inc/stb_image_write.h
    inc/wall.h
    inc/window.h
    inc/world.h)

set(project_sources
    src/finite_state_machine.cpp
    src/game.cpp
    src/glad.c
    src/main.cpp
    src/menu_state.cpp
    src/renderer_2D.cpp
    src/rigid_body_2D.cpp
    src/rigid_body_simulator.cpp
    src/shader.cpp
    src/shader_loader.cpp
    src/stb_image_write.cpp
    src/wall.cpp
    src/window.cpp
    src/world.cpp)

qt5_add_resources(project_sources qrc/rigid_body_simulator.qrc)

qt5_wrap_ui(project_headers_moc ${project_ui})

qt5_wrap_cpp(project_sources_moc
             inc/game.h
             inc/rigid_body_simulator.h)

add_definitions(-DUNICODE -D_UNICODE)

add_executable(${PROJECT_NAME} ${project_headers} ${project_sources} ${project_sources_moc} ${project_headers_moc})

target_link_libraries(${PROJECT_NAME} PUBLIC
                      general   Qt5::Core Qt5::Gui Qt5::Widgets
                      debug     debug/glfw3
                      optimized release/glfw3)
