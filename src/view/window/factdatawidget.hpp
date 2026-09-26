#ifndef FACTDATAWIDGET_HPP
#define FACTDATAWIDGET_HPP

#include "../widgets/formdatawidget.hpp"

#include <QWidget>

class QLineEdit;
class QListWidget;
class QSpinBox;

namespace openck {

class FormComponents;

class FactDataWidget : public QWidget, public FormDataWidget
{
    Q_OBJECT

public:
    explicit FactDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~FactDataWidget() override;

    void loadSession() override;
    bool validateSession(QString* error) override;
    void applySession() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // FACTDATAWIDGET_HPP
