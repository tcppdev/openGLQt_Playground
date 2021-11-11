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

#include <QQuickFramebufferObject>
// #include <QQuickView>

#include <iostream>

class MyFrameBufferObject : public QQuickFramebufferObject
{
    Q_OBJECT

    Q_PROPERTY(float azimuth READ azimuth WRITE setAzimuth NOTIFY azimuthChanged)
    Q_PROPERTY(float elevation READ elevation WRITE setElevation NOTIFY elevationChanged)
    Q_PROPERTY(float distance READ distance WRITE setDistance NOTIFY distanceChanged)

public:
    explicit MyFrameBufferObject(QQuickItem *parent = 0);
    Renderer *createRenderer() const Q_DECL_OVERRIDE;

    float azimuth() const;
    float distance() const;
    float elevation() const;
    float delta_x() const;
    float delta_y() const;
    int mouse_angle() const;
    void trigger_redraw();

signals:
    void azimuthChanged(float azimuth);
    void distanceChanged(float distance);
    void elevationChanged(float elevation);

public slots:
    void setAzimuth(float azimuth);
    void setDistance(float distance);
    void setElevation(float elevation);
    void request_redraw() { update(); }

protected:
    void mousePressEvent(QMouseEvent *e) override {

        mouse_pressed_ = true;
        mouse_angle_delta_ = 0;  // disable zooming when clicking
        
        // Initial mouse coordinates:
        mouse_x_ = e->x();
        mouse_y_ = e->y();
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
    }

    void wheelEvent(QWheelEvent *event) override {
        mouse_angle_delta_ = event->angleDelta().y();  // Update rendering
        update();
    }

private:
    float m_azimuth;
    float m_elevation;
    float m_distance;

    bool mouse_pressed_ = false;
    int mouse_x_ = 0;  // Current mouse x coordinate (relative to widget)
    int mouse_y_ = 0;  // Current mouse x coordinate (relative to widget)
    int mouse_angle_delta_ = 0;  // Current mouse scroll delta angle
    float delta_x_pos_ = 0;  // Change in mouse x position
    float delta_y_pos_ = 0;  // Change in mouse x position

};

#endif // MYFRAMEBUFFEROBJECT_H
