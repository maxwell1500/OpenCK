#ifndef SOUNDDATAWIDGET_HPP
#define SOUNDDATAWIDGET_HPP

#include <QWidget>

class QLineEdit;
class QSpinBox;

namespace openck {

class FormComponents;

class SoundDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SoundDataWidget(void* recordPtr, FormComponents* components,
                             QWidget* parent = nullptr);
    ~SoundDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // SOUNDDATAWIDGET_HPP
