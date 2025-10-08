cmake_minimum_required(VERSION 3.16)
project(openGLQt_Qt6_WebAssembly VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components for WebAssembly
find_package(Qt6 REQUIRED COMPONENTS 
    Core Widgets OpenGL Qml Quick Charts
)

# Qt already bundles FreeType for WebAssembly builds

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

# Eigen header-only library (mirrors meson's eigen_dep)
add_library(eigen INTERFACE)
target_include_directories(eigen INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/subprojects/eigen-3.4.0>
)

# Create alias to match meson dependency
add_library(eigen_dep ALIAS eigen)

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

# Link Qt6 libraries and Eigen header-only library (mirrors meson deps_common)
target_link_libraries(openGLQt_Qt6_WebAssembly PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::OpenGL
    Qt6::Charts
    eigen_dep  # Like meson's eigen_dep in deps_common
    # Qt6 already includes bundled FreeType
)

# WebAssembly specific flags for Qt 6.9.3
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_link_options(openGLQt_Qt6_WebAssembly PRIVATE
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sASYNCIFY=1
        --shell-file=${CMAKE_SOURCE_DIR}/wasm_shell.html
        -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']
        -sEXPORTED_FUNCTIONS=['_main']
        -sTOTAL_MEMORY=268435456
        -sSTACK_SIZE=65536
        -sFULL_ES2=1
        -sFULL_ES3=1
        -sMAX_WEBGL_VERSION=2
        -sMIN_WEBGL_VERSION=1
        "--preload-file=${CMAKE_SOURCE_DIR}/shaders@/shaders"
        "--preload-file=${CMAKE_SOURCE_DIR}/resources/objects@/resources/objects"
        -sASSERTIONS=1
        -sEXPORT_ALL=1
        -O2
    )
    
    # Add compile flags for EM_ASM_ debug functions
    target_compile_options(openGLQt_Qt6_WebAssembly PRIVATE
        -sASSERTIONS=1
    )
    
    # Set Qt WebAssembly environment variables for better WebGL compatibility
    target_compile_definitions(openGLQt_Qt6_WebAssembly PRIVATE
        QT_QPA_PLATFORM=wasm
        QT_LOGGING_RULES=qt.qpa.gl.debug=true
    )
endif()

qt6_finalize_target(openGLQt_Qt6_WebAssembly)
