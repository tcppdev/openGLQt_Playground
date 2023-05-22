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

    // Q_INIT_RESOURCE(assets);

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

    // Test chart view object
    QQuickItem *test_chart_view = graphics_window->findChild<QQuickItem*>("test_chart");
    // This = chartview wrapper
    ChartWrapper chart_object = ChartWrapper(test_chart_view);
    chart_object.create_line("my_test_line", "The blue line");
    QTimer* timer_ = new QTimer();
    timer_->setInterval(50);

    // // Setup timer trigger to plot update method
    QObject::connect(timer_, SIGNAL(timeout()), &chart_object, SLOT(update_line_data()));  

    // // Start update timer
    timer_->start();
    
    return app.exec();
}

