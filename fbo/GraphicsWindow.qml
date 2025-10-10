import QtQuick 6.0
import QtQuick.Controls 6.0
import QtCharts 6.0
import MyRenderLibrary 42.0

ApplicationWindow {
    id: root
    width: 900
    height: 900
    objectName: "graphics_window"

    color: "white" //  "lightblue"

    MyFrame {
        id: renderer
        width: root.width
        height: root.height
        anchors.left: root.left
        anchors.top: root.top
        anchors.leftMargin: 25
        anchors.topMargin: 25
        smooth: true

        Text {
            text: "Testing text"
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 10
        }
        
        // Mobile pinch-to-zoom support (doesn't interfere with mouse)
        PinchHandler {
            id: pinchHandler
            target: null  // Only detect pinch, don't transform the view
            
            property real lastScale: 1.0
            
            onActiveChanged: {
                if (active) {
                    lastScale = 1.0
                    console.log("Pinch started")
                }
            }
            
            onScaleChanged: {
                if (active) {
                    // Calculate incremental scale change
                    var scaleChange = scale / lastScale
                    
                    // Convert to mouse wheel delta (log scale for smooth zoom)
                    // Conservative multiplier for gentle mobile zoom
                    var wheelDelta = Math.log(scaleChange) * 80
                    
                    // Clamp to prevent extreme values
                    wheelDelta = Math.max(-30, Math.min(30, wheelDelta))
                    
                    renderer.handlePinchZoom(wheelDelta)
                    console.log("Pinch scale:", scale.toFixed(3), "delta:", wheelDelta.toFixed(2))
                    
                    lastScale = scale
                }
            }
        }
        
        // Mobile tap-to-select support
        // Note: Drag is handled automatically by Qt's touch-to-mouse synthesis
        TapHandler {
            id: tapHandler
            acceptedDevices: PointerDevice.TouchScreen  // Only touch, not mouse
            acceptedButtons: Qt.LeftButton
            gesturePolicy: TapHandler.DragThreshold  // Won't fire if drag distance exceeded
            
            onTapped: function(eventPoint) {
                console.log("Tap detected at:", eventPoint.position.x, eventPoint.position.y)
                renderer.handleTouchTap(eventPoint.position.x, eventPoint.position.y)
            }
        }

    }

    ChartView {
        id: chartView
        objectName: "test_chart"
        anchors.top: renderer.top
        anchors.right: renderer.right
        width: renderer.width/4
        height: renderer.height/4
        animationOptions: ChartView.NoAnimation
        backgroundColor: "#CCFFFFFF"
        legend.font.pointSize: 14
        legend.visible: true
    }
    
    Timer {
        id: render_control  // control refresh rate of our scene
        interval: 10 // [ms]
        running: true
        repeat: true
        onTriggered: { renderer.request_redraw() } 
    }

//    MyFrame {
//        id: rendererVeh
//        width: 900
//        height: 900
//        anchors.left: renderer.right
//        anchors.top: renderer.top
//        anchors.leftMargin: 25
//        // anchors.topMargin: 25
//        smooth: true
//        center_to_vehicle: true
//    }

    // Timer {
    //     id: render_control_veh  // control refresh rate of our scene
    //     interval: 10 // [ms]
    //     running: true
    //     repeat: true
    //     onTriggered: { rendererVeh.request_redraw() } 
    // }

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
}
