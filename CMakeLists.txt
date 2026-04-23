cmake_minimum_required(VERSION 3.10)
project(SpaceInvadersEmulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==================== CPU 8080 TESTS (sans dépendances externes) ====================
file(GLOB_RECURSE CPU_SOURCES "src/cpu8080.cpp")

add_executable(cpu8080_tests 
    tests/test_cpu8080.cpp
    ${CPU_SOURCES}
)
target_include_directories(cpu8080_tests PRIVATE include)
target_compile_definitions(cpu8080_tests PRIVATE DEBUG_MODE=1)

# ==================== SPACE INVADERS EMULATOR DEMO (sans GUI) ====================
file(GLOB_RECURSE SI_SOURCES 
    "src/cpu8080.cpp"
    "src/shift_register.cpp"
    "src/video.cpp"
    "src/input.cpp"
    "src/emulator.cpp"
)

add_executable(si_emulator_demo
    src/main_si_demo.cpp
    ${SI_SOURCES}
)
target_include_directories(si_emulator_demo PRIVATE include)
target_compile_definitions(si_emulator_demo PRIVATE DEBUG_MODE=1)

# ==================== SPACE INVADERS GUI (Raylib + rlImGui) ====================
file(GLOB_RECURSE SI_GUI_SOURCES 
    "src/cpu8080.cpp"
    "src/shift_register.cpp"
    "src/video.cpp"
    "src/input.cpp"
    "src/emulator.cpp"
    "src/main_gui.cpp"
)

# ImGui sources from third_party (utilisés par rlImGui)
set(IMGUI_SOURCES 
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
)

# rlImGui source (inclut son propre backend imgui_impl_raylib)
set(RLIMGUI_SOURCES 
    third_party/rlImGui/rlImGui.cpp
)

add_executable(space_invaders_gui
    ${SI_GUI_SOURCES}
    ${IMGUI_SOURCES}
    ${RLIMGUI_SOURCES}
)
target_include_directories(space_invaders_gui PRIVATE 
    include
    third_party/imgui
    third_party/imgui/backends
    third_party/rlImGui
    "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/include"
)
target_link_libraries(space_invaders_gui PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/lib/raylib.lib"
)
target_compile_definitions(space_invaders_gui PRIVATE DEBUG_MODE=1)

# Copy raylib.dll and glfw3.dll to output directory (dépendances runtime)
add_custom_command(TARGET space_invaders_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/raylib.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/glfw3.dll"
        $<TARGET_FILE_DIR:space_invaders_gui>
)

# ==================== INSTALLATION ====================
install(TARGETS cpu8080_tests si_emulator_demo space_invaders_gui DESTINATION bin)