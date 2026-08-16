#ifndef WORLDSPACEDATAWIDGET_HPP
#define WORLDSPACEDATAWIDGET_HPP

#include <QWidget>

namespace openck {
class FormComponents;
}

class Data;

class WorldspaceDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WorldspaceDataWidget(void* recordPtr, openck::FormComponents* components,
                                  Data* data, QWidget* parent = nullptr);
    ~WorldspaceDataWidget() override;

private:
    void* m_recordPtr;
    Data* m_data;
};

#endif // WORLDSPACEDATAWIDGET_HPP
