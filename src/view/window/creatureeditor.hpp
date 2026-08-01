#ifndef CREATUREEDITOR_HPP
#define CREATUREEDITOR_HPP

#include <QWidget>

class QLineEdit;
class QSpinBox;

namespace openck {

class FormComponents;

class CreatureDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CreatureDataWidget(void* recordPtr, FormComponents* components,
                                QWidget* parent = nullptr);
    ~CreatureDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // CREATUREEDITOR_HPP
