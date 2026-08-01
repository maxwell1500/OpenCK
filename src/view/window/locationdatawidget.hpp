#ifndef LOCATIONDATAWIDGET_HPP
#define LOCATIONDATAWIDGET_HPP

#include <QWidget>

namespace openck {
class FormComponents;
}

class LocationDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LocationDataWidget(void* recordPtr, openck::FormComponents* components,
                                QWidget* parent = nullptr);
    ~LocationDataWidget() override;

private:
    void* m_recordPtr;
};

#endif // LOCATIONDATAWIDGET_HPP
