#ifndef PATHS_H_
#define PATHS_H_

#ifdef _WIN32
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
namespace fs = boost::filesystem;
#elif defined(__EMSCRIPTEN__)
// WebAssembly/Emscripten - use simple string paths
#include <string>
namespace fs {
    class path {
        std::string p;
    public:
        path() = default;
        path(const std::string& s) : p(s) {}
        path(const char* s) : p(s) {}
        path operator/(const path& other) const {
            return path(p + "/" + other.p);
        }
        std::string string() const { return p; }
        operator std::string() const { return p; }
    };
    path current_path() { return path("."); }
}
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#ifdef __EMSCRIPTEN__
// WebAssembly - use preloaded virtual filesystem paths
fs::path ROOT_PROJECT_DIRECTORY = fs::path("/");
fs::path SHADERS_PATH = fs::path("/shaders");  
fs::path RESOURCES_PATH = fs::path("/resources");  
fs::path ASSETS_PATH = RESOURCES_PATH / "objects";
#else
fs::path ROOT_PROJECT_DIRECTORY = fs::current_path();
fs::path SHADERS_PATH = ROOT_PROJECT_DIRECTORY / "shaders";  
fs::path RESOURCES_PATH = ROOT_PROJECT_DIRECTORY / "resources";  
fs::path ASSETS_PATH = RESOURCES_PATH / "objects";
#endif


// Shaders

// Model loading
fs::path MODEL_VS = SHADERS_PATH / "model_loading.vs";
fs::path MODEL_FS = SHADERS_PATH / "model_loading.fs";

// Cube map
fs::path CUBEMAP_VS = SHADERS_PATH / "cubemap.vs";
fs::path CUBEMAP_FS = SHADERS_PATH / "cubemap.fs";

// Billboard
#ifdef __EMSCRIPTEN__
fs::path BILLBOARD_VS = SHADERS_PATH / "billboard_wasm.vs";
#else
fs::path BILLBOARD_VS = SHADERS_PATH / "billboard.vs";
#endif
fs::path BILLBOARD_FS = SHADERS_PATH / "billboard.fs";
fs::path BILLBOARD_GS = SHADERS_PATH / "billboard.gs";

// Delaunay Triangulation
fs::path DELAUNAY_2_5D_VS = SHADERS_PATH / "delaunay_2_5D.vs";
fs::path DELAUNAY_2_5D_FS = SHADERS_PATH / "delaunay_2_5D.fs";

// Point
#ifdef __EMSCRIPTEN__
fs::path POINT_VS = SHADERS_PATH / "point_wasm.vs";
#else
fs::path POINT_VS = SHADERS_PATH / "point.vs";
#endif
fs::path POINT_FS = SHADERS_PATH / "point.fs";
fs::path POINT_GS = SHADERS_PATH / "point.gs";

// Polygon
fs::path POLYGON_VS = SHADERS_PATH / "polygon.vs";
fs::path POLYGON_FS = SHADERS_PATH / "polygon.fs";

// Text
fs::path TEXT_VS = SHADERS_PATH / "text.vs";
fs::path TEXT_FS = SHADERS_PATH / "text.fs";

// Ellipsoid
fs::path ELLIPSOID_VS = SHADERS_PATH / "ellipsoid.vs";
fs::path ELLIPSOID_FS = SHADERS_PATH / "ellipsoid.fs";

// Lines
#ifdef __EMSCRIPTEN__
fs::path LINE_VS = SHADERS_PATH / "line_shader_wasm.vs";
#else
fs::path LINE_VS = SHADERS_PATH / "line_shader.vs";
#endif
fs::path LINE_FS = SHADERS_PATH / "line_shader.fs";
fs::path LINE_GS = SHADERS_PATH / "line_shader.gs";

// Object Bounding Box
fs::path OBB_VS = SHADERS_PATH / "obb.vs";
fs::path OBB_FS = SHADERS_PATH / "obb.fs";

// Cubemap images
fs::path CUBEMAP_PATH = RESOURCES_PATH / "cubemaps" / "universe";

// Text font
fs::path TEXT_FONT_PATH = RESOURCES_PATH / "fonts" / "Antonio-Bold.ttf";

// Assets
fs::path BACKPACK_OBJ = ASSETS_PATH / "backpack" / "backpack.obj";
fs::path EARTH_OBJ = ASSETS_PATH / "natural_earth" / "natural_earth_110m.obj";
fs::path ROCKET_OBJ = ASSETS_PATH / "rocket_v1" / "12217_rocket_v1_l1.obj";
fs::path COASTLINE_CSV = RESOURCES_PATH / "filtered_coast.csv";

#endif
