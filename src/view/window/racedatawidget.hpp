#ifndef RACEDATAWIDGET_HPP
#define RACEDATAWIDGET_HPP

#include <QWidget>

namespace openck {

class FormComponents;

class RaceDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RaceDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~RaceDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // RACEDATAWIDGET_HPP
