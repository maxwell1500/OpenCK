#ifndef DIALDATAWIDGET_HPP
#define DIALDATAWIDGET_HPP

#include <QWidget>

namespace openck {

class FormComponents;

class DialDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DialDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~DialDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // DIALDATAWIDGET_HPP
