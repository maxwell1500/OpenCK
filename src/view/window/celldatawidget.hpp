#ifndef CELLDATAWIDGET_HPP
#define CELLDATAWIDGET_HPP

#include <QWidget>

namespace openck {

class FormComponents;

class CellDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CellDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~CellDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // CELLDATAWIDGET_HPP
