# NetVision - CMakeLists.txt
# Developer: MERO:TG@QP4RM

cmake_minimum_required(VERSION 3.16)
project(NetVision VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(NV_BUILD_SHARED "Build shared library" ON)
option(NV_BUILD_TESTS "Build test executables" ON)
option(NV_ANDROID "Build for Android" OFF)
option(NV_HEADLESS "Build headless (no GUI)" OFF)
option(NV_USE_STUB_CAPTURE "Use stub capture (no libpcap)" OFF)

set(NV_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(NV_IMGUI_DIR ${CMAKE_SOURCE_DIR}/lib/imgui)

# ============================================================
# Core C library (capture, graph, session, replay)
# ============================================================
if(NV_USE_STUB_CAPTURE OR WIN32 OR NV_ANDROID)
    set(NV_CAPTURE_SRC src/core/nv_capture_stub.c)
else()
    set(NV_CAPTURE_SRC src/core/nv_capture.c)
endif()

set(NV_CORE_SOURCES
    ${NV_CAPTURE_SRC}
    src/core/nv_graph.c
    src/core/nv_session.c
    src/core/nv_replay.c
)

if(NV_BUILD_SHARED)
    add_library(nvcore SHARED ${NV_CORE_SOURCES})
    target_compile_definitions(nvcore PRIVATE NV_BUILD_DLL)
else()
    add_library(nvcore STATIC ${NV_CORE_SOURCES})
endif()

target_include_directories(nvcore PUBLIC ${NV_INCLUDE_DIR})
if(NV_USE_STUB_CAPTURE OR WIN32 OR NV_ANDROID)
    if(WIN32)
        target_link_libraries(nvcore PRIVATE ws2_32)
    endif()
else()
    target_link_libraries(nvcore PRIVATE pcap pthread m)
endif()

set_target_properties(nvcore PROPERTIES
    OUTPUT_NAME "nvcore"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

if(WIN32)
    set_target_properties(nvcore PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
        IMPORT_PREFIX ""
        IMPORT_SUFFIX ".lib"
    )
endif()

# ============================================================
# ImGui library
# ============================================================
if(NOT NV_HEADLESS AND NOT NV_ANDROID)
    set(NV_IMGUI_SOURCES
        ${NV_IMGUI_DIR}/imgui.cpp
        ${NV_IMGUI_DIR}/imgui_draw.cpp
        ${NV_IMGUI_DIR}/imgui_tables.cpp
        ${NV_IMGUI_DIR}/imgui_widgets.cpp
        ${NV_IMGUI_DIR}/imgui_demo.cpp
        ${NV_IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${NV_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )

    add_library(imgui STATIC ${NV_IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC
        ${NV_IMGUI_DIR}
        ${NV_IMGUI_DIR}/backends
    )

    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
endif()

# ============================================================
# Engine executable (GLFW + ImGui desktop)
# ============================================================
if(NOT NV_HEADLESS AND NOT NV_ANDROID)
    set(NV_ENGINE_SOURCES
        src/engine/nv_engine.cpp
        src/engine/main.cpp
    )

    add_executable(netvision ${NV_ENGINE_SOURCES})
    target_include_directories(netvision PRIVATE
        ${NV_INCLUDE_DIR}
        ${NV_IMGUI_DIR}
        ${NV_IMGUI_DIR}/backends
    )
    target_link_libraries(netvision PRIVATE nvcore imgui glfw OpenGL::GL pthread)

    install(TARGETS netvision RUNTIME DESTINATION bin)
endif()

# ============================================================
# Android shared library (ImGui + OpenGL ES)
# ============================================================
if(NV_ANDROID)
    set(NV_ANDROID_SOURCES
        src/engine/nv_engine.cpp
        ${NV_IMGUI_DIR}/imgui.cpp
        ${NV_IMGUI_DIR}/imgui_draw.cpp
        ${NV_IMGUI_DIR}/imgui_tables.cpp
        ${NV_IMGUI_DIR}/imgui_widgets.cpp
        ${NV_IMGUI_DIR}/imgui_demo.cpp
    )

    add_library(netvision_android SHARED ${NV_ANDROID_SOURCES})
    target_include_directories(netvision_android PRIVATE
        ${NV_INCLUDE_DIR}
        ${NV_IMGUI_DIR}
        ${NV_IMGUI_DIR}/backends
    )
    target_link_libraries(netvision_android PRIVATE nvcore log android EGL GLESv3)
    target_compile_definitions(netvision_android PRIVATE NV_ANDROID=1)
endif()

# ============================================================
# Install
# ============================================================
install(TARGETS nvcore
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include/netvision)

# ============================================================
# Test target
# ============================================================
if(NV_BUILD_TESTS AND NOT NV_ANDROID)
    add_executable(nv_test_graph tests/test_graph.c)
    target_include_directories(nv_test_graph PRIVATE ${NV_INCLUDE_DIR})
    target_link_libraries(nv_test_graph PRIVATE nvcore)

    add_executable(nv_test_session tests/test_session.c)
    target_include_directories(nv_test_session PRIVATE ${NV_INCLUDE_DIR})
    target_link_libraries(nv_test_session PRIVATE nvcore)

    add_executable(nv_test_replay tests/test_replay.c)
    target_include_directories(nv_test_replay PRIVATE ${NV_INCLUDE_DIR})
    target_link_libraries(nv_test_replay PRIVATE nvcore)
endif()

message(STATUS "NetVision v${PROJECT_VERSION} - MERO:TG@QP4RM")
message(STATUS "  Shared lib:  ${NV_BUILD_SHARED}")
message(STATUS "  Headless:    ${NV_HEADLESS}")
message(STATUS "  Android:     ${NV_ANDROID}")
message(STATUS "  Tests:       ${NV_BUILD_TESTS}")
