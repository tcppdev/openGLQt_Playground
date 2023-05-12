#include <chartwrapper.h>

#include <iostream>
#include <random>

#include <QString>

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

    QtCharts::QAbstractSeries* serie = get_line_ptr("my_test_line");

    std::vector<float> y_values;

    if(QtCharts::QLineSeries *line_serie = qobject_cast<QtCharts::QLineSeries *>(serie)){
        static std::default_random_engine e;
        static std::uniform_real_distribution<> dis(0, 3);
        line_serie->clear();
        for(int i=0; i < 14; i++){
            float y_value = dis(e);
            y_values.push_back(y_value);
            line_serie->append(i, y_value);
        }
    }

    float y_max = *std::max_element(y_values.begin(), y_values.end());
    resize_plot(y_max);
}

void ChartWrapper::resize_plot(float y_max)
{
    QMetaObject::invokeMethod(qml_chart_object_, "axisX", Qt::DirectConnection,
                            Q_RETURN_ARG(QtCharts::QAbstractAxis*, axis_x_));
    QMetaObject::invokeMethod(qml_chart_object_, "axisY", Qt::DirectConnection,
                              Q_RETURN_ARG(QtCharts::QAbstractAxis*, axis_y_));

    axis_x_->setRange(0, 14);
    axis_y_->setRange(0, y_max);
}