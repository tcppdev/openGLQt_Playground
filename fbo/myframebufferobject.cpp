#include "myframebufferobject.h"

#include <cmath>
#include <Eigen/Core>
#include <QElapsedTimer>
#include <QTimer>

// #include <meshrenderer.h>
#include <paths.h>
#include <model.h>
#include <camera_two.h>
#include <orbital_camera.h>
#include <shader.h>
#include <line.h>
#include <polygon.h>
#include <point.h>
#include <cube_map.h>
#include <text.h>
#include <delaunay_2_5D.h>
#include <ellipsoid.h>
#include <OBB.h>

// #include <mesh.h>
#include <string>
#include <vector>
#include <memory>

// ADDED
#include <QOpenGLContext>  // Optional
#include <QOpenGLFunctions_3_3_Core>

#include <QQuickWindow>
#include <QQuickView>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>

const float EARTH_RADIUS = 6371000; // [m]

Eigen::Vector3f sph_to_cart(float radius, float theta, float inc)  // all angles in degrees
{   // convert cylindrical to cartesian coordinates
    double x = radius*std::sin(glm::radians(theta))*std::cos(glm::radians(inc));
    double y = radius*std::sin(glm::radians(theta))*std::sin(glm::radians(inc));
    double z = radius*std::cos(glm::radians(theta));

    return Eigen::Vector3f(x, y, z);
};

std::vector<std::string> lineToVectorOfStrings(std::string line)
{
    std::stringstream ss(line);
    std::vector<std::string> strVec;
    while (ss.good()) {
        std::string substr;
        std::getline(ss, substr, ',');
        strVec.push_back(substr);
    }
    return strVec;
}

void getMapCoords(const std::string& coastLinePath,
                  std::vector<double>& vecLats,
                  std::vector<double>& vecLongs,
                  std::vector<int>& indexes)
{
    // The vector arguments are empty at this point.

    // Read csv file:
    std::ifstream is(coastLinePath);
    if (!is.is_open()) {
        std::string err = "Could not find coastline path at loc: ";
        err += coastLinePath;
        throw std::invalid_argument(err);
    }

    std::string line("");
    while (std::getline(is, line)) {
        std::vector<std::string> csv = lineToVectorOfStrings(line);
        vecLats.push_back(std::stod(csv[0]));
        vecLongs.push_back(std::stod(csv[1]));
        indexes.push_back(std::stoi(csv[2]));
    }
};

class MyFrameBufferObjectRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions_3_3_Core
{
public:
    MyFrameBufferObjectRenderer()
    {
        initializeOpenGLFunctions();   // Initialises current context

        // Create shaders
        const char* vertex_shader_path = MODEL_VS.c_str(); 
        const char* fragment_shader_path = MODEL_FS.c_str();
        m_shader = new Shader(vertex_shader_path, fragment_shader_path);

        // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
        stbi_set_flip_vertically_on_load(true);
        
        // Create models
        std::string model_path = ASSETS_PATH / "natural_earth/natural_earth_110m.obj"; 
        m_model =  std::make_unique<Model>(model_path);

        // Ellipsoid earth 
        m_ellipsoid_earth = std::make_unique<Ellipsoid>(glm::vec3(0.98*EARTH_RADIUS, 0.98*EARTH_RADIUS, 0.98*EARTH_RADIUS), 40, 40);

        // 
        model_path = ASSETS_PATH / "backpack/backpack.obj"; 
        other_model =  std::make_unique<Model>(model_path);

        // Get our rocket
        std::string rocket_path = ASSETS_PATH / "/rocket_v1/12217_rocket_v1_l1.obj";
        m_rocket =  std::make_unique<Model>(rocket_path);

        // Ellipsoid
        m_ellipsoid =  std::make_unique<Ellipsoid>(glm::vec3(0.2*EARTH_RADIUS, 0.2*EARTH_RADIUS, 0.2*EARTH_RADIUS), 40, 40);

        // OBB 
        m_obb =  std::make_unique<OBB>(glm::vec3(-0.02*EARTH_RADIUS, -0.02*EARTH_RADIUS, -0.1*EARTH_RADIUS), 
                                       glm::vec3(0.02*EARTH_RADIUS, 0.02*EARTH_RADIUS, 0.25*EARTH_RADIUS), Color::TRANSPARENT_WHITE);


        // Camera 
        m_camera = std::make_unique<OrbitalCamera>(glm::vec3(0.0f, 0.0f, 0.0f), 3*EARTH_RADIUS, 1.0*EARTH_RADIUS);  //new CameraT(glm::vec3(0.0f, 0.0f, 8.0f));
        
        // My (static) line
        std::vector<std::vector<Eigen::Vector3f>> the_lines;
        std::vector<Eigen::Vector3f> the_coordinates;
        for (std::size_t i_theta = 0; i_theta <= 360; ++i_theta) {
            Eigen::Vector3f coordinate = sph_to_cart(m_radius, i_theta, m_inc);
            the_coordinates.push_back(coordinate);
        }
        the_lines.push_back(the_coordinates);
        m_circular_line =  std::make_unique<Line>(the_lines, 10); 

        // 
        // My polygon
        std::vector<std::vector<Eigen::Vector3f>> the_polygons;
        std::vector<Eigen::Vector3f> polygon_1_coordinates;
        polygon_1_coordinates.emplace_back(EARTH_RADIUS, EARTH_RADIUS, EARTH_RADIUS);
        polygon_1_coordinates.emplace_back(EARTH_RADIUS, 0, EARTH_RADIUS);
        // polygon_1_coordinates.emplace_back(4, -1, 1);v
        polygon_1_coordinates.emplace_back(0, 0, EARTH_RADIUS);
        polygon_1_coordinates.emplace_back(0, EARTH_RADIUS, EARTH_RADIUS);
        polygon_1_coordinates.emplace_back(EARTH_RADIUS, EARTH_RADIUS, EARTH_RADIUS);
        the_polygons.push_back(polygon_1_coordinates);

        std::vector<Eigen::Vector3f> polygon_2_coordinates;
        polygon_2_coordinates.emplace_back(0, EARTH_RADIUS, 0);
        polygon_2_coordinates.emplace_back(0, EARTH_RADIUS, EARTH_RADIUS);
        polygon_2_coordinates.emplace_back(0, 0, EARTH_RADIUS);
        the_polygons.push_back(polygon_2_coordinates);

        m_polygon =  std::make_unique<Polygon>(the_polygons); 

        // Draw circle on sphere?
        // Method 1: Flat circle
        // Choose center point (ECEF) and convert it to NED frame
        // Draw circle in NE coordinates
        // Convert coordinates back to ECEF
        // Draw the polygon (using triangle fan)

        // Method 2: Unfilled curve circle
        // Choose center point (ECEF)
        // Find point on sphere that make a (projected) circle
        // https://en.wikipedia.org/wiki/Circle_of_a_sphere
        // Push points outward from earth using NED frame
        // Draw line connecting points

        // My points
        std::vector<GeoPoint> the_points;
        the_points.push_back(GeoPoint(Eigen::Vector3f(EARTH_RADIUS, EARTH_RADIUS, -EARTH_RADIUS), "This is a\nnew line\nthis one is a long line"));
        the_points.push_back(GeoPoint(Eigen::Vector3f(EARTH_RADIUS, -EARTH_RADIUS, -EARTH_RADIUS), "This is point 2"));
        the_points.push_back(GeoPoint(Eigen::Vector3f(-EARTH_RADIUS, EARTH_RADIUS, EARTH_RADIUS), "This is another point"));
        the_points.push_back(GeoPoint(Eigen::Vector3f(-EARTH_RADIUS, -EARTH_RADIUS, EARTH_RADIUS), "a\nb\nc"));
        m_points = std::make_unique<Point>(the_points, 0.1*EARTH_RADIUS, Symbol::CIRCLE);

        std::vector<double> lats;
        std::vector<double> longs;
        std::vector<int> indexes;
        getMapCoords(ROOT_PROJECT_DIRECTORY / "filtered_coast.csv", lats, longs, indexes);
        int current_index = -999;

        std::vector<ConstrainedDelaunayContourEdges> contour_edges;
        std::vector<std::vector<std::pair<double, double>>> delaunay_edges;
        std::vector<std::pair<double, double>> edge;
        
        for (std::size_t i = 0; i < lats.size(); ++i) {
            // TODO Maybe replace with the vector one of these
            int new_index = indexes[i];
            
            if (new_index == current_index || i == 0)
            {
                edge.push_back(std::make_pair(longs[i], lats[i]));
            }
            else {
                delaunay_edges.push_back(edge);
                ConstrainedDelaunayContourEdges contour_edge(delaunay_edges, false);
                contour_edges.push_back(contour_edge);

                edge.clear();
                delaunay_edges.clear();
                edge.push_back(std::make_pair(longs[i], lats[i]));
            }
            current_index = new_index;
        }

        delaunay_edges.push_back(edge);  // Push back last line
        ConstrainedDelaunayContourEdges contour_edge(delaunay_edges, false);
        contour_edges.push_back(contour_edge);

        m_projected_shapes = std::make_unique<Delaunay2_5D>(contour_edges, 1, 1, 5000, Color::RED, true);

        // My text
        m_text = std::make_unique<Text3D>("Awesome moving rocket", 0.0f, 0.0f, 0.0f, 1.0f/1200.0f);//1.0f/600.0f); 

        // My cubemap
        m_cubemap = std::make_unique<CubeMap>("path_to_cube_map");

        // start timer
        timer_.start();
        
        // m_window->resetOpenGLState();
    }

    ~MyFrameBufferObjectRenderer(){
        delete m_shader;
    }

    void synchronize(QQuickFramebufferObject *item) Q_DECL_OVERRIDE
    {
        m_window = item->window();

        MyFrameBufferObject *i = static_cast<MyFrameBufferObject *>(item);

        m_delta_x = i->delta_x();
        m_delta_y = -i->delta_y(); 
        m_mouse_delta_angle = i->mouse_angle();

        m_current_azimuth = i->azimuth(); // m_current_azimuth + i->delta_x();//
        m_current_elevation = i->elevation(); // m_current_elevation + i->delta_y(); 
        // m_current_distance = i->distance();
        m_current_center_to_vehicle = i->center_to_vehicle();

        // Line visibility toggle
        m_draw_line = i->line_visibility();

        // Process (right) click input
        m_click_toggle = i->mouse_click();

        m_window_width = i->get_window_width();
        m_window_height = i->get_window_height();

    }

    void render() Q_DECL_OVERRIDE
    {
        // m_render.render();
        // don't forget to enable shader before setting uniforms
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST); 
        // glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        
        // Rocket Center 
        float nMilliseconds = static_cast<float>(timer_.elapsed());
        float theta = nMilliseconds/100;  //  aol [deg]
        Eigen::Vector3f cord_r = sph_to_cart(m_radius, theta, m_inc);

        if (m_current_center_to_vehicle) {
            m_camera->set_camera_target(glm::vec3(cord_r[0], cord_r[1], cord_r[2]));
        }
        
        // view/projection transformations
        m_camera->process_mouse_scroll(m_mouse_delta_angle);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)m_window_width / (float)m_window_height, (float)0.001*EARTH_RADIUS, 10*EARTH_RADIUS);  // 
        // glm::mat4 projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, -50.0f, 50.0f);
        // for othographic projection zoom to work, scale the object using mouse scroll rather and 
        // changing the distance of the camera to the object (using the mouse scroll)

        m_camera->process_mouse_movements(m_delta_x, m_delta_y);
        glm::mat4 view = m_camera->get_view_matrix();
        
        ///////////////////////////////////
        // Evaluate incoming ray properties
        // See https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_custom.cpp
        glm::vec3 ray_ndc = m_click_toggle.second;
        glm::vec4 ray_clip_start = glm::vec4(ray_ndc.x, ray_ndc.y, -1.0f, 1.0f); // Ray vector start point in homogeneous clip coordinates
        glm::vec4 ray_clip_end = glm::vec4(ray_ndc.x, ray_ndc.y, 0.0f, 1.0f); // Ray vector end point in homogeneous clip coordinates
        
        // Faster way (just one inverse)
        glm::mat4 M = glm::inverse(projection * view);
        glm::vec4 ray_world_start = M * ray_clip_start; ray_world_start /= ray_world_start.w;
        glm::vec4 ray_world_end   = M * ray_clip_end  ; ray_world_end   /= ray_world_end.w;
        glm::vec4 ray_direction_world(ray_world_end - ray_world_start);

        glm::vec4 ray_world_origin = ray_world_start;
        ray_direction_world = glm::normalize(ray_direction_world);

        ///////////////////////////////////

        // Draw cubemap (draw first -> required as text are 2D objects)
        m_cubemap->draw(view, projection);

        m_shader->use();
        m_shader->setMat4("projection", projection);
        m_shader->setMat4("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        // model = glm::translate(model, glm::vec3(0.0f, 0.0f, m_current_distance)); // translate it down so it's at the center of the scene
        float earth_scaling = 1.0f;
        model = glm::scale(model, glm::vec3(earth_scaling, earth_scaling, earth_scaling));	// it's a bit too big for our scene, so scale it down
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));  // elevation rotation

        m_shader->setMat4("model", model);
        m_model->Draw(*m_shader);

        // render small earth
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.5*EARTH_RADIUS, 0.0f, 0.0f));  // elevation rotation
        // model = glm::translate(model, glm::vec3(0.0f, 0.0f, m_current_distance)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1000000.0f, 1000000.0f, 1000000.0f));	// it's a bit too big for our scene, so scale it down
        model = glm::rotate(model, glm::radians(m_current_azimuth), glm::vec3(0.0f, 1.0f, 0.0f));  // azimuth rotation 
        model = glm::rotate(model, glm::radians(m_current_elevation), glm::vec3(1.0f, 0.0f, 0.0f));  // elevation rotation
        // model = glm::translate(model, glm::vec3(5.0f, 0.0f, 0.0f));  // elevation rotation

        m_shader->setMat4("model", model);
        other_model->Draw(*m_shader);

        // Lets draw the line
        if (m_draw_line) {
            m_circular_line->draw(view, projection);
        }

        // Lets draw the polygon
        m_polygon->draw(view, projection);
        
        // Draw points
        if (m_click_toggle.first) {
            m_points->test_ray_tracing(view, projection, m_click_toggle.second);
            // m_click_toggle.first = false;  // desactivate mouse click
        }
        m_points->draw(view, projection);

        // Draw delaunay projection
        m_projected_shapes->draw(view, projection); 

        // Draw ellipsoid
        Eigen::Vector3f cord_ellipsoid = sph_to_cart(m_radius, theta/2, 135);
        glm::mat4 model_ellipsoid = glm::translate(glm::mat4(1.0f), glm::vec3(cord_ellipsoid[0], cord_ellipsoid[1], cord_ellipsoid[2]));  // 
        model_ellipsoid = glm::rotate(model_ellipsoid, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));  // y-rotation
        model_ellipsoid = glm::rotate(model_ellipsoid, glm::radians(-135.0f), glm::vec3(1.0f, 0.0f, 0.0f));  // inclination-rotation
        model_ellipsoid = glm::rotate(model_ellipsoid, glm::radians(theta), glm::vec3(0.0f, 1.0f, 0.0f));  // theta-rotation
        model_ellipsoid = glm::rotate(model_ellipsoid, glm::radians(m_current_azimuth), glm::vec3(0.0f, 1.0f, 0.0f));  // azimuth rotation 
        model_ellipsoid = glm::rotate(model_ellipsoid, glm::radians(m_current_elevation), glm::vec3(1.0f, 0.0f, 0.0f));  // elevation rotation
        model_ellipsoid = glm::scale(model_ellipsoid, glm::vec3(0.5*theta/360, theta/360, 0.5*theta/360));  // Scale is last (order of operation is reversed! scale -> rotate -> translate)
        m_ellipsoid->draw(view, projection, model_ellipsoid);

        // Draw ellispoid
        if (m_click_toggle.first) {
            if(m_ellipsoid->test_ray_tracing(ray_world_origin, ray_direction_world, model_ellipsoid)) {
                ellipse_toggled_ = true;
            }
            else {
                ellipse_toggled_ = false;
            }
        }

        if (ellipse_toggled_) { 
            m_ellipsoid->set_fill_color(Color::RED);
        }
        else {
            m_ellipsoid->set_fill_color(Color::BLUE);
        }

        m_ellipsoid_earth->draw(view, projection);

        /// Draw rocket
        // std::cout << theta << std::endl;

        // Change this to quaternion one day
        glm::mat4 model_rocket = glm::mat4(1.0f);
        model_rocket = glm::translate(model_rocket, glm::vec3(cord_r[0], cord_r[1], cord_r[2]));  // translate it
        model_rocket = glm::scale(model_rocket, glm::vec3(1000.0f)); // glm::vec3(0.1f)); // (0.001f));	// scale it down
        // rotations
        model_rocket = glm::rotate(model_rocket, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));  // y-rotation
        model_rocket = glm::rotate(model_rocket, glm::radians(-m_inc), glm::vec3(1.0f, 0.0f, 0.0f));  // inclination-rotation
        model_rocket = glm::rotate(model_rocket, glm::radians(theta), glm::vec3(0.0f, 1.0f, 0.0f));  // theta-rotation

        glm::mat4 model_obb = glm::mat4(1.0f);
        model_obb = glm::translate(model_obb, glm::vec3(cord_r[0], cord_r[1], cord_r[2]));  // translate it
        model_obb = glm::rotate(model_obb, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));  // y-rotation
        model_obb = glm::rotate(model_obb, glm::radians(-m_inc), glm::vec3(1.0f, 0.0f, 0.0f));  // inclination-rotation
        model_obb = glm::rotate(model_obb, glm::radians(theta), glm::vec3(0.0f, 1.0f, 0.0f));  // theta-rotation

        // Draw OBB
        if (m_click_toggle.first) {
            if(m_obb->test_ray_tracing(glm::vec3(ray_world_origin), glm::vec3(ray_direction_world), model_obb)) {
                obb_toggled_ = true;
            }
            else {
                obb_toggled_ = false;
            }
        }
        if (obb_toggled_) { m_obb->draw(view, projection, model_obb); };

        // Set shader properties
        m_shader->use();  
        m_shader->setMat4("model", model_rocket);
        m_rocket->Draw(*m_shader);

        /// Render text after cubemap (since its a 2D object)
        Eigen::Vector3f cord_text = sph_to_cart(1.05*m_radius, theta, m_inc);
        m_text->update_position(cord_text[0], cord_text[1], cord_text[2]);
        m_text->draw(view, projection);

        m_window->resetOpenGLState();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) Q_DECL_OVERRIDE
    {
        QOpenGLFramebufferObjectFormat format;
        format.setSamples(4);
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        return new QOpenGLFramebufferObject(size, format);
    }

private:
    // MeshRenderer m_render;
    QQuickWindow *m_window;    
    Shader* m_shader;
    // Shader* m_line_shader;
    std::unique_ptr<Model> m_model;

    std::unique_ptr<Model> other_model;
    std::unique_ptr<Model> m_rocket;
    std::unique_ptr<Ellipsoid> m_ellipsoid;
    std::unique_ptr<Ellipsoid> m_ellipsoid_earth;
    std::unique_ptr<OBB> m_obb;
    std::unique_ptr<Line> m_circular_line;
    std::unique_ptr<Polygon> m_polygon;
    std::unique_ptr<Delaunay2_5D> m_projected_shapes;
    std::unique_ptr<Point> m_points;
    std::unique_ptr<Text3D> m_text;
    std::unique_ptr<CubeMap> m_cubemap;
    std::unique_ptr<OrbitalCamera> m_camera;

    // Transforms 
    float m_current_azimuth = 0;
    float m_current_elevation = 0;
    // float m_current_distance = 0;
    float m_delta_x; 
    float m_delta_y;
    int m_mouse_delta_angle;

    bool m_current_center_to_vehicle = false;

    // Orbital line properties
    float m_radius = 1.2*EARTH_RADIUS;  // [m]
    float m_inc = 45;  // inclination angle [deg]

    QElapsedTimer timer_;

    // Line toggle
    bool m_draw_line = true;
    bool obb_toggled_ = false;  // control obb visibility control
    bool ellipse_toggled_ = false; // control ellipsoid color

    // Mouse clicking 
    std::pair<bool, glm::vec3> m_click_toggle = std::make_pair(false, glm::vec3(0.0f));

    // Window params
    qreal m_window_width;
    qreal m_window_height;
};

// MyFrameBufferObject implementation

MyFrameBufferObject::MyFrameBufferObject(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , m_azimuth(0.0)
    , m_elevation(0.0)
    , m_distance(0.0)
{
    setMirrorVertically(true);  // 
    setAcceptedMouseButtons(Qt::AllButtons);  // Need this to make sure mousePressEvent is called
    
    /// Trigger a redraw every 10 ms no matter what:
    // QTimer *redrawTimer = new QTimer(this);
    // QObject::connect(redrawTimer, &QTimer::timeout, this, &MyFrameBufferObject::trigger_redraw);
    // redrawTimer->start(10);
}

QQuickFramebufferObject::Renderer *MyFrameBufferObject::createRenderer() const
{
    return new MyFrameBufferObjectRenderer;
}

void MyFrameBufferObject::trigger_redraw()
{
    update();
}

qreal MyFrameBufferObject::get_window_height()
{
    return this->height();
}


qreal MyFrameBufferObject::get_window_width()
{
    return this->width();
}

void MyFrameBufferObject::set_line_visibility(bool visibility)
{
    line_visibility_ = visibility;
}

std::pair<bool, glm::vec3> MyFrameBufferObject::mouse_click() const
{
    return std::make_pair(mouse_click_, ray_ndc_);
}

float MyFrameBufferObject::azimuth() const
{
    return m_azimuth;
}

bool MyFrameBufferObject::center_to_vehicle() const
{
    return m_center_to_vehicle;
}

bool MyFrameBufferObject::line_visibility() const
{ 
    return line_visibility_;
}

float MyFrameBufferObject::distance() const
{
    return m_distance;
}

float MyFrameBufferObject::elevation() const
{
    return m_elevation;
}

float MyFrameBufferObject::delta_x()
{
    float delta_x_return = delta_x_pos_;

    if (delta_x_pos_ > 0 || delta_x_pos_ < 0)
    {
        delta_x_pos_ = 0;  // reset delta x after triggered
    }

    return delta_x_return;
}

float MyFrameBufferObject::delta_y()
{
    float delta_y_return = delta_y_pos_;

    if (delta_y_pos_ > 0 || delta_y_pos_ < 0)
    {
        delta_y_pos_ = 0;  // reset delta y after triggered
    }

    return delta_y_return;
}

int MyFrameBufferObject::mouse_angle()  
{
    int mouse_return = mouse_angle_delta_;

    if (mouse_angle_delta_ > 0 || mouse_angle_delta_ < 0)
    {
        mouse_angle_delta_ = 0;  // reset mouse zoom after triggered
    }

    return mouse_return;
}

void MyFrameBufferObject::setAzimuth(float azimuth)
{
    if (m_azimuth == azimuth)
        return;

    m_azimuth = azimuth;
    emit azimuthChanged(azimuth);
    update();
}

void MyFrameBufferObject::set_center_to_vehicle(bool center_to_vehicle)
{
    m_center_to_vehicle = center_to_vehicle;
    emit center_to_vehicle_changed();
    // update();
}

void MyFrameBufferObject::setDistance(float distance)
{
    if (m_distance == distance)
        return;

    m_distance = distance;
    emit distanceChanged(distance);
    update();
}

void MyFrameBufferObject::setElevation(float elevation)
{
    if (m_elevation == elevation)
        return;

    m_elevation = elevation;
    emit elevationChanged(elevation);
    update();
}
