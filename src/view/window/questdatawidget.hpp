#ifndef QUESTDATAWIDGET_HPP
#define QUESTDATAWIDGET_HPP

#include <QWidget>

namespace openck {

class FormComponents;

class QuestDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QuestDataWidget(void* recordPtr, FormComponents* components,
                             QWidget* parent = nullptr);
    ~QuestDataWidget() override;

private:
    void* m_recordPtr;
};

} // namespace openck

#endif // QUESTDATAWIDGET_HPP
