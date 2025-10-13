cmake_minimum_required(VERSION 3.16)
project(openGLQt_Qt6_WebAssembly VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent for downloading dependencies
include(FetchContent)

# Find Qt6 components for WebAssembly
find_package(Qt6 REQUIRED COMPONENTS 
    Core Widgets OpenGL Qml Quick Charts
)

# Enable Qt features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Include directories - same as meson build + Assimp for 3D models
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/general_inc
    ${CMAKE_CURRENT_SOURCE_DIR}/ext_inc
    ${CMAKE_CURRENT_SOURCE_DIR}/ext_inc/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/ext_inc/assimp
    ${CMAKE_CURRENT_SOURCE_DIR}/glad
    # Eigen as header-only (like meson does)
    ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/eigen-3.4.0
    # CDT (Constrained Delaunay Triangulation) library
    ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/CDT/CDT
    ${CMAKE_CURRENT_SOURCE_DIR}/fbo
    # Qt bundled FreeType includes (WebAssembly compatible)
)

# ==========================================
# Dependency Management (mirrors meson.build)
# ==========================================

# 1. Assimp dependency
find_package(assimp QUIET)
if(NOT assimp_FOUND)
    message(STATUS "System assimp not found, fetching from source...")
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.4.3
        GIT_SHALLOW TRUE
    )
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(assimp)
    set(assimp_dep assimp::assimp)
else()
    message(STATUS "Using system assimp")
    set(assimp_dep assimp::assimp)
endif()

# 2. Eigen dependency (header-only, mirrors meson's eigen_dep)
find_package(Eigen3 QUIET)
if(NOT Eigen3_FOUND)
    message(STATUS "System Eigen3 not found, checking subprojects...")
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/eigen-3.4.0)
        # Use local eigen from subprojects
        add_library(eigen INTERFACE)
        target_include_directories(eigen INTERFACE 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/subprojects/eigen-3.4.0>
        )
        add_library(eigen_dep ALIAS eigen)
        message(STATUS "Using local Eigen from subprojects/eigen-3.4.0")
    else()
        # Fetch eigen from git
        message(STATUS "Fetching Eigen from source...")
        FetchContent_Declare(
            eigen
            GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
            GIT_TAG 3.4.0
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(eigen)
        add_library(eigen_dep ALIAS eigen)
    endif()
else()
    message(STATUS "Using system Eigen3")
    add_library(eigen_dep ALIAS Eigen3::Eigen)
endif()

# 3. FreeType dependency (mirrors meson's freetype_dep from subproject)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # For WebAssembly, use Qt's bundled FreeType
    message(STATUS "WebAssembly build: Using Qt's bundled FreeType")
    add_library(freetype_dep INTERFACE)
    target_compile_definitions(freetype_dep INTERFACE USE_QT_FREETYPE)
else()
    # For native builds, fetch FreeType from git (like meson wrap file)
    find_package(Freetype QUIET)
    if(NOT Freetype_FOUND)
        message(STATUS "System FreeType not found, fetching from source...")
        FetchContent_Declare(
            freetype
            GIT_REPOSITORY https://github.com/freetype/freetype.git
            GIT_TAG VER-2-13-3
            GIT_SHALLOW TRUE
        )
        set(FT_DISABLE_ZLIB OFF CACHE BOOL "" FORCE)
        set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
        set(FT_DISABLE_PNG OFF CACHE BOOL "" FORCE)
        set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
        set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(freetype)
        add_library(freetype_dep ALIAS freetype)
    else()
        message(STATUS "Using system FreeType")
        add_library(freetype_dep ALIAS Freetype::Freetype)
    endif()
endif()

# Source files - exactly as in meson build + essential Assimp sources for WebAssembly
set(SOURCES
    fbo/main.cpp
    fbo/myframebufferobject.cpp
    fbo/chartwrapper.cpp
    ext_inc/stb_image.cpp
)

# For WebAssembly, we'll build essential Assimp sources directly
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # Add minimal Assimp implementation - we'll create this next
    list(APPEND SOURCES
        wasm_src/assimp_minimal.cpp
    )
endif()

# Header files
set(HEADERS
    fbo/myframebufferobject.h
    fbo/chartwrapper.h
    general_inc/utilities.h
    general_inc/shader.h
    general_inc/camera_two.h
    general_inc/orbital_camera.h
    general_inc/line.h
    general_inc/mesh.h
    general_inc/model.h
    general_inc/point.h
    general_inc/polygon.h
    general_inc/cube_map.h
    general_inc/delaunay_2_5D.h
    general_inc/ellipsoid.h
    general_inc/OBB.h
    general_inc/text.h
    general_inc/billboard.h
    general_inc/paths.h
    ext_inc/stb_image.h
)

# QML files
set(QML_FILES
    fbo/main.qml
    fbo/GraphicsWindow.qml 
    fbo/CameraControls.qml
)

# Create executable
qt6_add_executable(openGLQt_Qt6_WebAssembly
    MANUAL_FINALIZATION
    ${SOURCES}
    ${HEADERS}
)

# Add QML resources (simple approach for WebAssembly)
qt6_add_resources(openGLQt_Qt6_WebAssembly "qml_resources"
    PREFIX "/"
    FILES ${QML_FILES}
)

# Link Qt6 libraries and dependencies (mirrors meson deps_common)
target_link_libraries(openGLQt_Qt6_WebAssembly PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::OpenGL
    Qt6::Charts
    ${assimp_dep}    # Like meson's assimp_dep
    eigen_dep        # Like meson's eigen_dep
    freetype_dep     # Like meson's freetype_dep
)

# WebAssembly specific flags for Qt 6.9.3
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_link_options(openGLQt_Qt6_WebAssembly PRIVATE
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sASYNCIFY=1
        -sUSE_FREETYPE=1
        # --shell-file=${CMAKE_SOURCE_DIR}/wasm_shell.html
        -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']
        -sEXPORTED_FUNCTIONS=['_main']
        # Mobile-friendly memory settings: start small, grow as needed
        -sINITIAL_MEMORY=67108864
        -sMAXIMUM_MEMORY=536870912
        -sSTACK_SIZE=65536
        -sFULL_ES2=1
        -sFULL_ES3=1
        -sMAX_WEBGL_VERSION=2
        -sMIN_WEBGL_VERSION=1
        "--preload-file=${CMAKE_SOURCE_DIR}/shaders@/shaders"
        "--preload-file=${CMAKE_SOURCE_DIR}/resources/objects@/resources/objects"
        "--preload-file=${CMAKE_SOURCE_DIR}/resources/cubemaps@/resources/cubemaps"
        "--preload-file=${CMAKE_SOURCE_DIR}/resources/fonts@/resources/fonts"
        -sASSERTIONS=1
        -sEXPORT_ALL=1
        -O2
    )
    
    # Add compile flags for EM_ASM_ debug functions and FreeType support
    target_compile_options(openGLQt_Qt6_WebAssembly PRIVATE
        -sASSERTIONS=1
        -sUSE_FREETYPE=1
    )
    
    # Set Qt WebAssembly environment variables for better WebGL compatibility
    target_compile_definitions(openGLQt_Qt6_WebAssembly PRIVATE
        QT_QPA_PLATFORM=wasm
        QT_LOGGING_RULES=qt.qpa.gl.debug=true
    )
endif()

qt6_finalize_target(openGLQt_Qt6_WebAssembly)
