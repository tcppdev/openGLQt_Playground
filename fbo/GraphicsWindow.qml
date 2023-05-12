/****************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company.
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

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtCharts 2.1
import MyRenderLibrary 42.0

ApplicationWindow {
    id: root
    width: 1000
    height: 1000
    objectName: "graphics_window"

    color: "white" //  "lightblue"
        
    // MyFrame {
    //     id: renderer0
    //     x: 750
    //     width: 750
    //     height: 750
    //     anchors.top: root.top
    //     anchors.leftMargin: 25
    //     anchors.topMargin: 25
    //     smooth: true
    // }

    MyFrame {
        id: renderer
        width: 900
        height: 900
        anchors.left: root.left
        anchors.top: root.top
        anchors.leftMargin: 25
        anchors.topMargin: 25
        smooth: true
        
        // ChartView {
        //     width: 400
        //     height: 300
        //     theme: ChartView.ChartThemeBrownSand
        //     antialiasing: true

        //     PieSeries {
        //         id: pieSeries
        //         PieSlice { label: "eaten"; value: 94.9 }
        //         PieSlice { label: "not yet eaten"; value: 5.1 }
        //     }
        // }

        Text {
            text: "Testing text"
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
        }

        // MouseArea { 
        //     anchors.fill: parent
        //     enabled: false // set to true and it blocks events from FBO
        //     acceptedButtons: Qt.LeftButton
        //     //onClicked: flashyThing.flash()
        // }

        // Rectangle {
        //     id: flashyThing
        //     anchors.fill: parent
        //     opacity: 0

        //     color: "yellow"

        //     Text {
        //         id: flashyThingText
        //         anchors.centerIn: parent
        //         text: "Clicked!"
        //         font.pointSize: 40
        //     }

        //     function flash() {
        //         flashyAnimation.restart()
        //     }

        //     ParallelAnimation {
        //         id: flashyAnimation

        //         NumberAnimation {
        //             target: flashyThingText
        //             properties: "font.pointSize"
        //             from: 20
        //             to: 60
        //             duration: 2000
        //             easing.type: Easing.OutCubic
        //         }

        //         SequentialAnimation {
        //             PropertyAction {
        //                 target: flashyThing
        //                 properties: "opacity"
        //                 value: 1
        //             }

        //             NumberAnimation {
        //                 target: flashyThing
        //                 properties: "opacity"
        //                 to: 0
        //                 easing.type: Easing.InCubic
        //                 duration: 1500
        //             }
        //         }
        //     }

        // }
    }

    ChartView {
        id: chartView
        objectName: "test_chart"
        anchors.top: renderer.top
        anchors.right: renderer.right
        width: renderer.width/3
        height: renderer.height/3
        animationOptions: ChartView.NoAnimation
        backgroundColor: "red" // "transparent"
        legend.font.pointSize: 14
        legend.visible: true
    }

    MyFrame {
        id: rendererVeh
        width: 900
        height: 900
        anchors.left: renderer.right
        anchors.top: renderer.top
        anchors.leftMargin: 25
        // anchors.topMargin: 25
        smooth: true
        center_to_vehicle: true
    }

    Timer {
        id: render_control  // control refresh rate of our scene
        interval: 10 // [ms]
        running: true
        repeat: true
        onTriggered: { renderer.request_redraw() } 
    }

    Timer {
        id: render_control_veh  // control refresh rate of our scene
        interval: 10 // [ms]
        running: true
        repeat: true
        onTriggered: { rendererVeh.request_redraw() } 
    }

    Switch {
        id: my_toggle
        checked : true
        anchors.left: root.left
        
        Text {text: "Show line"; color: "black"; anchors.left: my_toggle.right;
            anchors.verticalCenter: my_toggle.verticalCenter}

        onClicked: {
            renderer.set_line_visibility(checked)
        }

    }

    CameraControls {
        camera: renderer  // sets camera property (property var camera) using  MyFrame instance with id renderer
        anchors.bottom: renderer.bottom
        anchors.horizontalCenter: renderer.horizontalCenter
    }


    // ParallelAnimation {
    //     loops: Animation.Infinite
    //     running: false
    //     SequentialAnimation {
    //         loops: Animation.Infinite

    //         NumberAnimation {
    //             target: renderer
    //             properties: "x"
    //             from: 50
    //             to: root.width - renderer.width
    //             duration: 4000
    //         }
    //         NumberAnimation {
    //             target: renderer
    //             properties: "x"
    //             from: root.width - renderer.width
    //             to: 0
    //             duration: 5000
    //         }
    //         NumberAnimation {
    //             target: renderer
    //             properties: "x"
    //             from: 0
    //             to: 50
    //             duration: 1000
    //         }
    //     }

    //     SequentialAnimation {
    //         loops: Animation.Infinite
    //         NumberAnimation {
    //             target: renderer
    //             properties: "y"
    //             from: 100
    //             to: 0
    //             duration: 1500
    //         }

    //         NumberAnimation {
    //             target: renderer
    //             properties: "y"
    //             from: 0
    //             to: root.height - renderer.height
    //             duration: 5000
    //         }

    //         NumberAnimation {
    //             target: renderer
    //             properties: "y"
    //             from: root.height - renderer.height
    //             to: 100
    //             duration: 4000
    //         }

    //     }

    //     SequentialAnimation {
    //         loops: Animation.Infinite

    //         PauseAnimation {
    //             duration: 10000
    //         }

    //         RotationAnimation {
    //             target: renderer
    //             properties: "rotation"
    //             from: 0
    //             to: 45
    //             duration: 500
    //             easing.type: Easing.OutBack
    //         }

    //         RotationAnimation {
    //             target: renderer
    //             properties: "rotation"
    //             to: -45
    //             duration: 1000
    //             easing.type: Easing.OutBack
    //         }

    //         RotationAnimation {
    //             target: renderer
    //             properties: "rotation"
    //             to: 0
    //             duration: 500
    //             easing.type: Easing.OutBack
    //         }
    //     }

    // }
}

