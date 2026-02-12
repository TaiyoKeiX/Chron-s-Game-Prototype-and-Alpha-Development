cmake_minimum_required(VERSION 3.16)
project(BioStandalone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(bio_tests
    main.cpp
    BioTypes.h
    BioTypes.cpp
    BioUnitBase.h
    BioUnitBase.cpp
    BioGridManager.h
    BioGridManager.cpp
    BioGameMode.h
    BioGameMode.cpp
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_dx11.cpp
    third_party/imgui/backends/imgui_impl_win32.cpp
)

target_include_directories(bio_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
)

target_link_libraries(bio_tests PRIVATE d3d11 dxgi d3dcompiler)
