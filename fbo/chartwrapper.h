
#include <Eigen/Core>
#include <QObject>

#include <QMetaObject>
#include <QAbstractAxis>
#include <QAbstractSeries>
#include <QLineSeries>
#include <QQuickItem>

#include <unordered_map>

/// Wrapper class used to handle plot data
class ChartWrapper : public QObject {
    Q_OBJECT

    public:
        ChartWrapper() = delete;

        /// Constructor for the chart wrapper
        ///
        /// \param qml_chart_object QML ChartView object to wrap
        ChartWrapper(QQuickItem* const qml_chart_object);
        ~ChartWrapper();


        // Create a lineseries
        void create_line(std::string unique_id, std::string label);

        // Get pointer to a lineseries
        QtCharts::QAbstractSeries* get_line_ptr(std::string unique_id);
        
        // Resize the plot
        void resize_plot(float y_max);

    public slots:

        void update_line_data(); // std::string unique_id); // , double x, double y);


    private:
        
        QQuickItem* const qml_chart_object_ = nullptr;
        QtCharts::QAbstractAxis* axis_x_ = nullptr;
        QtCharts::QAbstractAxis* axis_y_ = nullptr;

        int line_type_;

        // Line inside chart
        std::unordered_map<std::string, QtCharts::QAbstractSeries*> line_series_;
};
