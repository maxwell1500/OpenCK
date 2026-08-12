#ifndef FACTDATAWIDGET_HPP
#define FACTDATAWIDGET_HPP

#include <QWidget>

class QLineEdit;
class QListWidget;
class QSpinBox;

namespace openck {

class FormComponents;

class FactDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FactDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~FactDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // FACTDATAWIDGET_HPP
