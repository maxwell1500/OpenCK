#ifndef TES3RECORDDATAWIDGET_HPP
#define TES3RECORDDATAWIDGET_HPP

#include <QWidget>

namespace openck {

class FormComponents;

// Read-only identity view plus raw-subrecord inspector for generic TES3
// (Morrowind) records. Specialized Morrowind editors are out of scope;
// this surfaces what the parser round-tripped so users can inspect it.
class Tes3RecordDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit Tes3RecordDataWidget(void* recordPtr, FormComponents* components,
                                  QWidget* parent = nullptr);
    ~Tes3RecordDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // TES3RECORDDATAWIDGET_HPP
