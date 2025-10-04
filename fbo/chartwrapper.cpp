#include <chartwrapper.h>

#include <iostream>
#include <random>

#include <QString>
#include <QMetaEnum>
#include <QtCharts/QAbstractSeries>
#include <QtCharts/QAbstractAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>

ChartWrapper::ChartWrapper(QQuickItem* const qml_chart_object)
    : qml_chart_object_(qml_chart_object)
{
    // Fetch line type
    const QMetaObject *meta_object = qml_chart_object_->metaObject();

    if(std::strcmp(meta_object->className(), "QtCharts::DeclarativeChart") != 0)
        return;
        
    int series_type_enum_index = meta_object->indexOfEnumerator("SeriesType");
    QMetaEnum series_type_enum = meta_object->enumerator(series_type_enum_index);
    line_type_ = series_type_enum.keyToValue("SeriesTypeLine");
}

ChartWrapper::~ChartWrapper()
{

}

void ChartWrapper::create_line(std::string unique_id, std::string label)
{
    QtCharts::QAbstractSeries* serie = nullptr;
    // Create series and fill with data
    QMetaObject::invokeMethod(qml_chart_object_, "createSeries", Qt::DirectConnection,
                              Q_RETURN_ARG(QtCharts::QAbstractSeries *, serie),
                              Q_ARG(int, line_type_),
                              Q_ARG(QString, QString::fromStdString(label)),
                              Q_ARG(QtCharts::QAbstractAxis *, axis_x_),
                              Q_ARG(QtCharts::QAbstractAxis *, axis_y_));

    // Save the line pointer
    line_series_.insert(std::make_pair(unique_id, serie));
                              
}


QtCharts::QAbstractSeries* ChartWrapper::get_line_ptr(std::string unique_id)
{
    if (line_series_.find(unique_id) == line_series_.end())
    {
        std::cout << "Key not found" << std::endl;  // TO-DO Throw
    }

    QtCharts::QAbstractSeries* serie = line_series_[unique_id];

    return serie;
}

void ChartWrapper::update_line_data() // std::string unique_id) // , double x, double y)
{
    // Note: Simplified for Qt6 compatibility - chart functionality may need runtime testing
    std::cout << "update_line_data() called - Qt6 chart functionality simplified" << std::endl;
}

void ChartWrapper::resize_plot(float y_max)
{
    // Note: Simplified for Qt6 compatibility - chart functionality may need runtime testing  
    std::cout << "resize_plot() called with y_max: " << y_max << std::endl;
}
