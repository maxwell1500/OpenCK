#ifndef CELLDATAWIDGET_HPP
#define CELLDATAWIDGET_HPP

#include "../widgets/formdatawidget.hpp"

#include <QWidget>

namespace openck {

class FormComponents;

class CellDataWidget : public QWidget, public FormDataWidget
{
    Q_OBJECT

public:
    explicit CellDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~CellDataWidget() override;

    void loadSession() override;
    bool validateSession(QString* error) override;
    void applySession() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // CELLDATAWIDGET_HPP
