#ifndef RACEDATAWIDGET_HPP
#define RACEDATAWIDGET_HPP

#include "../widgets/formdatawidget.hpp"

#include <QWidget>

class QListWidget;
class QSpinBox;

namespace openck {

class FormComponents;

class RaceDataWidget : public QWidget, public FormDataWidget
{
    Q_OBJECT

public:
    explicit RaceDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~RaceDataWidget() override;

    void loadSession() override;
    bool validateSession(QString* error) override;
    void applySession() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // RACEDATAWIDGET_HPP
