#ifndef PACKDATAWIDGET_HPP
#define PACKDATAWIDGET_HPP

#include <QWidget>

namespace openck {
class FormComponents;
}

class PackDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PackDataWidget(void* recordPtr, openck::FormComponents* components,
                            QWidget* parent = nullptr);
    ~PackDataWidget() override;

private:
    void* m_recordPtr;
};

#endif // PACKDATAWIDGET_HPP
