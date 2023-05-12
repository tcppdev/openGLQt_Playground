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
// #include "glad/glad.h"  // ADDED

#include <QGuiApplication>
#include <QQuickView>
#include <QQmlEngine>
#include <QSurfaceFormat>
#include <QQmlApplicationEngine>
#include <QtWidgets/QApplication>
#include <QtCharts/QChartView>


#include "myframebufferobject.h"


#include <QMetaObject>
#include <QAbstractAxis>
#include <QAbstractSeries>
#include <QLineSeries>
#include <chartwrapper.h>

#include <random>
#include <QTimer>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Q_INIT_RESOURCE(assets);

    QSurfaceFormat format;
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    qmlRegisterType<MyFrameBufferObject>("MyRenderLibrary", 42, 0, "MyFrame");

    // Load window(s)
    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:///qml/main.qml"));

    QQuickWindow* graphics_window = engine.rootObjects()[0]->findChild<QQuickWindow*>("graphics_window");
    graphics_window->show();

    // 
    QQuickItem *test_chart_view = graphics_window->findChild<QQuickItem*>("test_chart");

    // Initialise pointers
    // QtCharts::QAbstractAxis *axis_x = nullptr;
    // QtCharts::QAbstractAxis *axis_y = nullptr;
    // QtCharts::QAbstractSeries *serie = nullptr;

    // Fetch correct line type
    // const QMetaObject *meta_object = test_chart_view->metaObject();
    // if(std::strcmp(meta_object->className(), "QtCharts::DeclarativeChart") != 0)
    //     return;
    // int series_type_enum_index = meta_object->indexOfEnumerator("SeriesType");
    // QMetaEnum series_type_enum = meta_object->enumerator(series_type_enum_index);
    // int line_type = series_type_enum.keyToValue("SeriesTypeLine");

    // This = chartview wrapper
    ChartWrapper chart_object = ChartWrapper(test_chart_view);
    // ChartLine* line_ptr = chart_object->create_line(std::string unique_id, std::string label, std::vector<float> x, std::vector<float> y);
    chart_object.create_line("my_test_line", "this is a line");
    // ChartLine* line_ptr = chart_object->get_line_ptr(std::string unique_id);
    // line_ptr->update_line();

    // this->create_lines(std::vector<std::string> line_labels);
    // this->update_line(std::string line_label, float x, float y) // mode append
    // this->update_line(std::string line_label, std::vector<float> x, std::vector<float> y, append=True); // 
    // this->resize_plot(); // Resize to fit lines

    // Create series and fill with data
    // QMetaObject::invokeMethod(test_chart_view, "createSeries", Qt::DirectConnection,
    //                           Q_RETURN_ARG(QtCharts::QAbstractSeries *, serie),
    //                           Q_ARG(int, line_type),
    //                           Q_ARG(QString, "serie from c++"),
    //                           Q_ARG(QtCharts::QAbstractAxis *, axis_x),
    //                           Q_ARG(QtCharts::QAbstractAxis *, axis_y));

    // if(QtCharts::QLineSeries *line_serie = qobject_cast<QtCharts::QLineSeries *>(serie)){
    //     static std::default_random_engine e;
    //     static std::uniform_real_distribution<> dis(0, 3);
    //     for(int i=0; i < 14; i++){
    //         line_serie->append(i, dis(e));
    //     }
    // }
    
    // Pull the generated axes
    // QMetaObject::invokeMethod(test_chart_view, "axisX", Qt::DirectConnection,
    //                         Q_RETURN_ARG(QtCharts::QAbstractAxis *, axis_x));
    // QMetaObject::invokeMethod(test_chart_view, "axisY", Qt::DirectConnection,
    //                           Q_RETURN_ARG(QtCharts::QAbstractAxis *, axis_y));
    // axis_x->setRange(0, 14);
    // axis_y->setRange(0, 5);

    // Connections
    // QObject::connect(backend, SIGNAL(compass_mode_changed(bool)),
    //                  heading_dial_pilot_object_ptr, SLOT(set_compass_mode(bool)));  

    QTimer* timer_ = new QTimer();
    timer_->setInterval(500);

    // // Setup timer trigger to plot update method
    QObject::connect(timer_, SIGNAL(timeout()), &chart_object, SLOT(update_line_data()));  

    // // Start update timer
    timer_->start();


    // Let's create the chart

    
    return app.exec();
}

