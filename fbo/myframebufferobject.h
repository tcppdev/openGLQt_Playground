#ifndef MYFRAMEBUFFEROBJECT_H
#define MYFRAMEBUFFEROBJECT_H

#include <Eigen/Core>

#include <QQuickFramebufferObject>
#include <glm/glm.hpp>

#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif


class MyFrameBufferObject : public QQuickFramebufferObject
{
    Q_OBJECT

    Q_PROPERTY(float azimuth READ azimuth WRITE setAzimuth NOTIFY azimuthChanged)
    Q_PROPERTY(float elevation READ elevation WRITE setElevation NOTIFY elevationChanged)
    Q_PROPERTY(float distance READ distance WRITE setDistance NOTIFY distanceChanged)
    Q_PROPERTY(bool center_to_vehicle READ center_to_vehicle WRITE set_center_to_vehicle NOTIFY center_to_vehicle_changed)

public:
    explicit MyFrameBufferObject(QQuickItem *parent = 0);
    Renderer *createRenderer() const Q_DECL_OVERRIDE;

    float azimuth() const;
    float distance() const;
    float elevation() const;
    bool center_to_vehicle() const;
    float delta_x();
    float delta_y();
    int mouse_angle();
    bool line_visibility() const;
    void trigger_redraw();
    std::pair<bool, glm::vec3> mouse_click() const;
    void set_mouse_click(bool state);

    qreal get_window_height();
    qreal get_window_width();

signals:
    void azimuthChanged(float azimuth);
    void distanceChanged(float distance);
    void elevationChanged(float elevation);
    void center_to_vehicle_changed();

public slots:
    void setAzimuth(float azimuth);
    void setDistance(float distance);
    void setElevation(float elevation);
    void set_center_to_vehicle(bool center_to_vehicle);
    void request_redraw() { update(); }
    void set_line_visibility(bool visibility);
    
    // Mobile pinch zoom handler
    void handlePinchZoom(qreal scaleDelta) {
        // Convert pinch scale delta to mouse wheel delta
        mouse_angle_delta_ = static_cast<int>(scaleDelta);
        update();
    }

    // Desktop zoom handler
    void wheelEvent(QWheelEvent *event) override {
        mouse_angle_delta_ = event->angleDelta().y();  // Update rendering
        update();
    }

    // Mobile tap handler
    void handleTouchTap(qreal x, qreal y) {
        // Simulate right-click for selection (like mousePressEvent with RightButton)
        mouse_x_ = x;
        mouse_y_ = y;

        mouse_angle_delta_ = 0;  // Disable zooming when tapping
        mouse_pressed_ = false;  // Disable moving when clicking
        mouse_click_ = true;
        screen_tapped_ = true;

        // Convert to 3d normalised device coordinates
        // https://antongerdelan.net/opengl/raycasting.html
        qreal width_window = this->width();
        qreal height_window = this->height();
        float x_r = (2.0f * mouse_x_) / width_window - 1.0f;
        float y_r = 1.0f - (2.0f * mouse_y_) / height_window;
        float z_r = 1.0f;
        ray_ndc_ = glm::vec3(x_r, y_r, z_r);
    }

protected:
    void mousePressEvent(QMouseEvent *e) override {

        mouse_pressed_ = true;
        mouse_angle_delta_ = 0;  // disable zooming when clicking
        
        // Initial mouse coordinates:
        mouse_x_ = e->x();
        mouse_y_ = e->y();

        if (e->button() == Qt::RightButton) 
        {
            mouse_pressed_ = false;  // Disable moving when clicking
            mouse_click_ = true;

            // Convert to 3d normalised device coordinates
            // https://antongerdelan.net/opengl/raycasting.html
            qreal width_window = this->width();
            qreal height_window = this->height();
            float x = (2.0f * mouse_x_) / width_window - 1.0f;
            float y = 1.0f - (2.0f * mouse_y_) / height_window;
            float z = 1.0f;
            ray_ndc_ = glm::vec3(x, y, z);
        }
        
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        if (mouse_pressed_) {
            // std::cout << "Mouse moving" << std::endl;

            delta_x_pos_ = e->x() - mouse_x_;
            delta_y_pos_ = e->y() - mouse_y_;

            // std::cout << delta_x_pos_ << std::endl;
            
            // Reset mouse position
            mouse_x_ = e->x();
            mouse_y_ = e->y();
            update();  // Update rendering (trigger re-draw from QQuickItem base class)
         }

    }

    void mouseReleaseEvent(QMouseEvent *e) override {
        // std::cout << "Mouse released" << std::endl;
        mouse_pressed_ = false; 
        delta_x_pos_ = 0;
        delta_y_pos_ = 0;
        
        if (e->button() == Qt::RightButton || screen_tapped_) 
        {
            mouse_click_ = false;  // release mouse
        }

        if (screen_tapped_){
            mouse_click_ = true; // keep mouse_click true for one render cycle
            screen_tapped_ = false; // reset tap
        }
    }

private:
    float m_azimuth;
    float m_elevation;
    float m_distance;
    bool m_center_to_vehicle = false;

    bool mouse_pressed_ = false;
    bool mouse_click_ = false; // Mouse clicking (with right click)
    bool screen_tapped_ = false; // Screen tapped (mobile)
    int mouse_x_ = 0;  // Current mouse x coordinate (relative to widget)
    int mouse_y_ = 0;  // Current mouse x coordinate (relative to widget)
    int mouse_angle_delta_ = 0;  // Current mouse scroll delta angle
    float delta_x_pos_ = 0;  // Change in mouse x position
    float delta_y_pos_ = 0;  // Change in mouse x position

    // Mouse ray
    glm::vec3 ray_ndc_;   // Ray vector in normalised device coordinates

    // Toggle
    bool line_visibility_ = true;
};

#endif // MYFRAMEBUFFEROBJECT_H
