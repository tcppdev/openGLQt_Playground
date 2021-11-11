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

#include <meshrenderer.h>
#include <model.h>
#include <camera_two.h>
#include <orbital_camera.h>
#include <shader.h>
#include <line.h>
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
        m_render.initialize();

        // Create shaders
        const char* vertex_shader_path = "/home/t.clar/Repos/openGLQt/shaders/1.model_loading.vs";
        const char* fragment_shader_path = "/home/t.clar/Repos/openGLQt/shaders/1.model_loading.fs";
        m_shader = new Shader(vertex_shader_path, fragment_shader_path);

        // Line shader
        // const char* vertex_line_path = "/home/t.clar/Repos/integrating-qq2-with-opengl/shaders/line_shader.vs";
        // const char* fragment_line_path = "/home/t.clar/Repos/integrating-qq2-with-opengl/shaders/line_shader.fs";
        // m_line_shader = new Shader(vertex_line_path, fragment_line_path);

        // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
        stbi_set_flip_vertically_on_load(true);
        
        // Create model
        std::string model_path = "/home/t.clar/Repos/openGLQt/resources/objects/backpack/backpack.obj";
        m_model = new Model(model_path);

        // Camera 
        m_camera = new OrbitalCamera(glm::vec3(0.0f, 0.0f, 0.0f));  //new CameraT(glm::vec3(0.0f, 0.0f, 8.0f));
        
        // m_window->resetOpenGLState();
    }

    ~MyFrameBufferObjectRenderer(){
        delete m_shader;
        delete m_model;
        delete m_camera;
    }

    void synchronize(QQuickFramebufferObject *item) Q_DECL_OVERRIDE
    {
        m_window = item->window();

        MyFrameBufferObject *i = static_cast<MyFrameBufferObject *>(item);
        m_render.setAzimuth(i->azimuth());
        m_render.setElevation(i->elevation());
        m_render.setDistance(i->distance());

        // 
        m_delta_x = i->delta_x();
        m_delta_y = i->delta_y(); 
        m_mouse_delta_angle = i->mouse_angle();

        m_current_azimuth = i->azimuth(); // m_current_azimuth + i->delta_x();//
        m_current_elevation = i->elevation(); // m_current_elevation + i->delta_y(); 
        // m_current_distance = i->distance();

    }

    void render() Q_DECL_OVERRIDE
    {
        // m_render.render();
        // don't forget to enable shader before setting uniforms
        glEnable(GL_DEPTH_TEST);
        // glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        m_shader->use();

        // view/projection transformations
        m_camera->process_mouse_scroll(m_mouse_delta_angle);
        // glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f, -100.0f, 100.0f);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)600 / (float)600, 0.1f, 100.0f);

        m_camera->process_mouse_movements(m_delta_x, m_delta_y);
        glm::mat4 view = m_camera->get_view_matrix();
        m_shader->setMat4("projection", projection);
        m_shader->setMat4("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        // model = glm::translate(model, glm::vec3(0.0f, 0.0f, m_current_distance)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
        model = glm::rotate(model, glm::radians(m_current_azimuth), glm::vec3(0.0f, 1.0f, 0.0f));  // azimuth rotation 
        
        model = glm::rotate(model, glm::radians(m_current_elevation), glm::vec3(1.0f, 0.0f, 0.0f));  // elevation rotation

        m_shader->setMat4("model", model);
        m_model->Draw(*m_shader);

        // Lets draw a line
        // Define coordinates
        std::vector<Eigen::Vector3f> the_coordinates;
        float radius = 3;  // [m]
        float theta = 0;  //  aol [deg]
        float inc = 45;  // inclination angle [deg]
        
        for (std::size_t i_theta = 0; i_theta <= 360; ++i_theta) {
            Eigen::Vector3f coordinate = sph_to_cart(radius, i_theta, inc);
            the_coordinates.push_back(coordinate);
        }

        Line my_circular_line(the_coordinates, view, projection, 10); 
        my_circular_line.draw();

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
    MeshRenderer m_render;
    QQuickWindow *m_window;    
    Shader* m_shader;
    // Shader* m_line_shader;
    Model* m_model;
    OrbitalCamera* m_camera;

    // Transforms 
    float m_current_azimuth = 0;
    float m_current_elevation = 0;
    // float m_current_distance = 0;
    float m_delta_x; 
    float m_delta_y;
    int m_mouse_delta_angle;
};

// MyFrameBufferObject implementation

MyFrameBufferObject::MyFrameBufferObject(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , m_azimuth(0.0)
    , m_elevation(0.0)
    , m_distance(0.0)
{
    setMirrorVertically(true);
    setAcceptedMouseButtons(Qt::AllButtons);  // Need this to make sure mousePressEvent is called
}

QQuickFramebufferObject::Renderer *MyFrameBufferObject::createRenderer() const
{
    return new MyFrameBufferObjectRenderer;
}

float MyFrameBufferObject::azimuth() const
{
    return m_azimuth;
}

float MyFrameBufferObject::distance() const
{
    return m_distance;
}

float MyFrameBufferObject::elevation() const
{
    return m_elevation;
}

float MyFrameBufferObject::delta_x() const
{
    return delta_x_pos_;
}

float MyFrameBufferObject::delta_y() const
{
    return delta_y_pos_;
}

int MyFrameBufferObject::mouse_angle() const 
{
    return mouse_angle_delta_;
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
