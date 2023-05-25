#ifndef PATHS_H_
#define PATHS_H_

#ifdef _WIN32
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
namespace fs = boost::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

fs::path ROOT_PROJECT_DIRECTORY = fs::current_path();
fs::path SHADERS_PATH = ROOT_PROJECT_DIRECTORY / "shaders";  

fs::path RESOURCES_PATH = ROOT_PROJECT_DIRECTORY / "resources";  
fs::path ASSETS_PATH = RESOURCES_PATH / "objects";


// Shaders

// Model loading
fs::path MODEL_VS = SHADERS_PATH / "model_loading.vs";
fs::path MODEL_FS = SHADERS_PATH / "model_loading.fs";

// Cube map
fs::path CUBEMAP_VS = SHADERS_PATH / "cubemap.vs";
fs::path CUBEMAP_FS = SHADERS_PATH / "cubemap.fs";

// Billboard
fs::path BILLBOARD_VS = SHADERS_PATH / "billboard.vs";
fs::path BILLBOARD_FS = SHADERS_PATH / "billboard.fs";
fs::path BILLBOARD_GS = SHADERS_PATH / "billboard.gs";

// Delaunay Triangulation
fs::path DELAUNAY_2_5D_VS = SHADERS_PATH / "delaunay_2_5D.vs";
fs::path DELAUNAY_2_5D_FS = SHADERS_PATH / "delaunay_2_5D.fs";

// Point
fs::path POINT_VS = SHADERS_PATH / "point.vs";
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
fs::path LINE_VS = SHADERS_PATH / "line_shader.vs";
fs::path LINE_FS = SHADERS_PATH / "line_shader.fs";
fs::path LINE_GS = SHADERS_PATH / "line_shader.gs";

// Object Bounding Box
fs::path OBB_VS = SHADERS_PATH / "obb.vs";
fs::path OBB_FS = SHADERS_PATH / "obb.fs";

// Cubemap images
fs::path CUBEMAP_PATH = RESOURCES_PATH / "cubemaps" / "universe";

// Text font
fs::path TEXT_FONT_PATH = RESOURCES_PATH / "fonts" / "Antonio-Bold.ttf";

#endif