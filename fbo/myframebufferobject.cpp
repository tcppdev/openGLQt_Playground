/****************************************************************************
**
** Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company.
** Author: Giuseppe D'Angelo
** Contact: info@kdab.com
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
// #include "glad/glad.h"  // ADDED  (include before anything else)

#include "myframebufferobject.h"

#include <cmath>
#include <Eigen/Core>
#include <QElapsedTimer>
#include <QTimer>

// #include <meshrenderer.h>
#include <model.h>
#include <camera_two.h>
#include <orbital_camera.h>
#include <shader.h>
#include <line.h>
#include <polygon.h>
#include <point.h>
#include <cube_map.h>
#include <text.h>

// #include <mesh.h>
#include <string>
#include <vector>
// ADDED
#include <QOpenGLContext>  // Optional
#include <QOpenGLFunctions_3_3_Core>

#include <QQuickWindow>
#include <QQuickView>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>

Eigen::Vector3f sph_to_cart(float radius, float theta, float inc)  // all angles in degrees
{   // convert cylindrical to cartesian coordinates
    double x = radius*std::sin(glm::radians(theta))*std::cos(glm::radians(inc));
    double y = radius*std::sin(glm::radians(theta))*std::sin(glm::radians(inc));
    double z = radius*std::cos(glm::radians(theta));

    return Eigen::Vector3f(x, y, z);
};

class MyFrameBufferObjectRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions_3_3_Core
{
public:
    MyFrameBufferObjectRenderer()
    {
        initializeOpenGLFunctions();   // Initialises current context
        // m_render.initialize();

        // Create shaders
        const char* vertex_shader_path = "/home/t.clar/Repos/openGLQt/shaders/1.model_loading.vs";
        const char* fragment_shader_path = "/home/t.clar/Repos/openGLQt/shaders/1.model_loading.fs";
        m_shader = new Shader(vertex_shader_path, fragment_shader_path);

        // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
        stbi_set_flip_vertically_on_load(true);
        
        // Create models
        std::string model_path = "/home/t.clar/Repos/openGLQt/resources/objects/natural_earth/natural_earth_110m.obj";
        m_model = new Model(model_path);

        // 
        small_earth = new Model(model_path);

        // Get our rocket
        std::string rocket_path = "/home/t.clar/Repos/openGLQt/resources/objects/rocket_v1/12217_rocket_v1_l1.obj";
        m_rocket = new Model(rocket_path);

        // Camera 
        m_camera = new OrbitalCamera(glm::vec3(0.0f, 0.0f, 0.0f));  //new CameraT(glm::vec3(0.0f, 0.0f, 8.0f));
        
        // My (static) line
        std::vector<Eigen::Vector3f> the_coordinates;
        for (std::size_t i_theta = 0; i_theta <= 360; ++i_theta) {
            Eigen::Vector3f coordinate = sph_to_cart(m_radius, i_theta, m_inc);
            the_coordinates.push_back(coordinate);
        }
        m_circular_line = new Line(the_coordinates, 10); 

        // My polygon
        std::vector<std::vector<Eigen::Vector3f>> the_polygons;
        std::vector<Eigen::Vector3f> polygon_1_coordinates;
        polygon_1_coordinates.emplace_back(3, 3, 3);
        polygon_1_coordinates.emplace_back(3, 0, 3);
        polygon_1_coordinates.emplace_back(0, 0, 3);
        polygon_1_coordinates.emplace_back(0, 3, 3);
        polygon_1_coordinates.emplace_back(3, 3, 3);
        the_polygons.push_back(polygon_1_coordinates);

        std::vector<Eigen::Vector3f> polygon_2_coordinates;
        polygon_2_coordinates.emplace_back(0, 3, 0);
        polygon_2_coordinates.emplace_back(0, 3, 3);
        polygon_2_coordinates.emplace_back(0, 0, 3);
        the_polygons.push_back(polygon_2_coordinates);

        m_polygon = new Polygon(the_polygons); 

        // My points
        std::vector<Eigen::Vector3f> the_points;
        the_points.push_back(Eigen::Vector3f(2.0f, 2.0f, -2.0f));
        the_points.push_back(Eigen::Vector3f(2.0f, -2.0f, -2.0f));
        the_points.push_back(Eigen::Vector3f(-2.0f, 2.0f, 2.0f));
        the_points.push_back(Eigen::Vector3f(-2.0f, -2.0f, 2.0f));
        m_points = new Point(the_points, 0.2f, Symbol::CIRCLE);

        // My text
        m_text = new Text3D("Awesome moving rocket", 0.0f, 0.0f, 0.0f, 1.0f/1200.0f);//1.0f/600.0f); 

        // My cubemap
        m_cubemap = new CubeMap("path_to_cube_map");

        // start timer
        timer_.start();
        
        // m_window->resetOpenGLState();
    }

    ~MyFrameBufferObjectRenderer(){
        delete m_shader;
        delete m_model;
        delete small_earth;
        delete m_camera;
    }

    void synchronize(QQuickFramebufferObject *item) Q_DECL_OVERRIDE
    {
        m_window = item->window();

        MyFrameBufferObject *i = static_cast<MyFrameBufferObject *>(item);
        // m_render.setAzimuth(i->azimuth());
        // m_render.setElevation(i->elevation());
        // m_render.setDistance(i->distance());

        // 
        m_delta_x = i->delta_x();
        m_delta_y = -i->delta_y(); 
        m_mouse_delta_angle = i->mouse_angle();

        m_current_azimuth = i->azimuth(); // m_current_azimuth + i->delta_x();//
        m_current_elevation = i->elevation(); // m_current_elevation + i->delta_y(); 
        // m_current_distance = i->distance();

        // Line visibility toggle
        m_draw_line = i->line_visibility();

    }

    void render() Q_DECL_OVERRIDE
    {
        // m_render.render();
        // don't forget to enable shader before setting uniforms
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST); 
        // glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        
        m_shader->use();

        // view/projection transformations
        m_camera->process_mouse_scroll(m_mouse_delta_angle);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)600 / (float)600, 0.005f, 100.0f);
        // glm::mat4 projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, -50.0f, 50.0f);
        // for othographic projection zoom to work, scale the object using mouse scroll rather and 
        // changing the distance of the camera to the object (using the mouse scroll)

        m_camera->process_mouse_movements(m_delta_x, m_delta_y);
        glm::mat4 view = m_camera->get_view_matrix();
        m_shader->setMat4("projection", projection);
        m_shader->setMat4("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        // model = glm::translate(model, glm::vec3(0.0f, 0.0f, m_current_distance)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.0000004f, 0.0000004f, 0.0000004f));	// it's a bit too big for our scene, so scale it down
        model = glm::rotate(model, glm::radians(m_current_azimuth), glm::vec3(0.0f, 1.0f, 0.0f));  // azimuth rotation 
        model = glm::rotate(model, glm::radians(m_current_elevation), glm::vec3(1.0f, 0.0f, 0.0f));  // elevation rotation

        m_shader->setMat4("model", model);
        m_model->Draw(*m_shader);

        // render small earth
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(4.0f, 0.0f, 0.0f));  // elevation rotation
        // model = glm::translate(model, glm::vec3(0.0f, 0.0f, m_current_distance)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.00000004f, 0.00000004f, 0.00000004f));	// it's a bit too big for our scene, so scale it down
        model = glm::rotate(model, glm::radians(m_current_azimuth), glm::vec3(0.0f, 1.0f, 0.0f));  // azimuth rotation 
        model = glm::rotate(model, glm::radians(m_current_elevation), glm::vec3(1.0f, 0.0f, 0.0f));  // elevation rotation
        // model = glm::translate(model, glm::vec3(5.0f, 0.0f, 0.0f));  // elevation rotation

        m_shader->setMat4("model", model);
        small_earth->Draw(*m_shader);

        // Lets draw the line
        if (m_draw_line) {
            m_circular_line->draw(view, projection);
        }

        // Lets draw the polygon
        m_polygon->draw(view, projection);
        
        // Draw points
        m_points->draw(view, projection);

        /// Draw rocket

        float nMilliseconds = static_cast<float>(timer_.elapsed());
        float theta = nMilliseconds/100;  //  aol [deg]
        // std::cout << theta << std::endl;

        glm::mat4 model_rocket = glm::mat4(1.0f);
        Eigen::Vector3f cord_r = sph_to_cart(m_radius, theta, m_inc);
        model_rocket = glm::translate(model_rocket, glm::vec3(cord_r[0], cord_r[1], cord_r[2]));  // translate it
        model_rocket = glm::scale(model_rocket, glm::vec3(0.001f)); // glm::vec3(0.1f)); // (0.001f));	// scale it down
        // rotations
        model_rocket = glm::rotate(model_rocket, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));  // y-rotation
        model_rocket = glm::rotate(model_rocket, glm::radians(-m_inc), glm::vec3(1.0f, 0.0f, 0.0f));  // inclination-rotation
        model_rocket = glm::rotate(model_rocket, glm::radians(theta), glm::vec3(0.0f, 1.0f, 0.0f));  // theta-rotation

        // Set shader properties
        m_shader->use();  
        m_shader->setMat4("model", model_rocket);
        m_rocket->Draw(*m_shader);

        // Draw cubemap (last)
        m_cubemap->draw(view, projection);

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
    Model* m_model;

    Model* small_earth;
    Model* m_rocket;
    Line* m_circular_line;
    Polygon* m_polygon;
    Point* m_points;
    Text3D* m_text;
    CubeMap* m_cubemap;
    OrbitalCamera* m_camera;

    // Transforms 
    float m_current_azimuth = 0;
    float m_current_elevation = 0;
    // float m_current_distance = 0;
    float m_delta_x; 
    float m_delta_y;
    int m_mouse_delta_angle;

    // Orbital line properties
    float m_radius = 3;  // [m]
    float m_inc = 45;  // inclination angle [deg]

    QElapsedTimer timer_;

    // Line toggle
    bool m_draw_line = true;
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

void MyFrameBufferObject::set_line_visibility(bool visibility)
{
    line_visibility_ = visibility;
}

float MyFrameBufferObject::azimuth() const
{
    return m_azimuth;
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
