#ifndef INFODATAWIDGET_HPP
#define INFODATAWIDGET_HPP

#include <QWidget>

namespace openck {

class FormComponents;

class InfoDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InfoDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~InfoDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // INFODATAWIDGET_HPP
