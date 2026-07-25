#ifndef NPCRECORDDATAWIDGET_HPP
#define NPCRECORDDATAWIDGET_HPP

#include <QWidget>

class QFormLayout;
class QLineEdit;
class QSpinBox;

namespace openck {

class FormComponents;

// NPC record data widget — specialized editor section for NPC-specific
// fields that aren't covered by the generic component grid.
// Appears below the component property grid in QtFormDialog.
class NpcRecordDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NpcRecordDataWidget(void* recordPtr, FormComponents* components,
                                 QWidget* parent = nullptr);
    ~NpcRecordDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // NPCRECORDDATAWIDGET_HPP
