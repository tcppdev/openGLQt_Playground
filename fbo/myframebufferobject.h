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

#ifndef MYFRAMEBUFFEROBJECT_H
#define MYFRAMEBUFFEROBJECT_H

#include <Eigen/Core>

#include <QQuickFramebufferObject>
#include <glm/glm.hpp>

#include <iostream>

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

        update();
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

        if (e->button() == Qt::RightButton) 
        {
            mouse_click_ = false;  // release mouse
        }
    }

    void wheelEvent(QWheelEvent *event) override {
        mouse_wheel_activated_ = true;
        mouse_angle_delta_ = event->angleDelta().y();  // Update rendering
        update();
    }

private:
    float m_azimuth;
    float m_elevation;
    float m_distance;
    bool m_center_to_vehicle = false;

    bool mouse_wheel_activated_ = false;
    bool mouse_pressed_ = false;
    bool mouse_click_ = false; // Mouse clicking (with right click)
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
