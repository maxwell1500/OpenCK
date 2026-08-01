#ifndef WORLDSPACEDATAWIDGET_HPP
#define WORLDSPACEDATAWIDGET_HPP

#include <QWidget>

namespace openck {
class FormComponents;
}

class WorldspaceDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WorldspaceDataWidget(void* recordPtr, openck::FormComponents* components,
                                  QWidget* parent = nullptr);
    ~WorldspaceDataWidget() override;

private:
    void* m_recordPtr;
};

#endif // WORLDSPACEDATAWIDGET_HPP
