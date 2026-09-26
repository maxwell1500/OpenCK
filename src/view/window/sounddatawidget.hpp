#ifndef SOUNDDATAWIDGET_HPP
#define SOUNDDATAWIDGET_HPP

#include "../widgets/formdatawidget.hpp"

#include <QWidget>

class QLineEdit;
class QSpinBox;

namespace openck {

class FormComponents;

class SoundDataWidget : public QWidget, public FormDataWidget
{
    Q_OBJECT

public:
    explicit SoundDataWidget(void* recordPtr, FormComponents* components,
                             QWidget* parent = nullptr);
    ~SoundDataWidget() override;

    void loadSession() override;
    bool validateSession(QString* error) override;
    void applySession() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // SOUNDDATAWIDGET_HPP
