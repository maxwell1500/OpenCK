#ifndef INFODATAWIDGET_HPP
#define INFODATAWIDGET_HPP

#include <QWidget>

class QTableWidget;
class QLineEdit;
class QPlainTextEdit;

namespace openck {

class FormComponents;

class InfoDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InfoDataWidget(void* recordPtr, FormComponents* components,
                            QWidget* parent = nullptr);
    ~InfoDataWidget() override;

private slots:
    void onAddCondition();
    void onRemoveCondition();
    void onConditionChanged();
    void onBrowseVoiceFile();
    void onPlayVoiceFile();
    void onCompileFragment();

private:
    void populateConditions();
    void syncConditionsToRecord();

    void* m_recordPtr;
    QTableWidget* m_condTable;
    QLineEdit* m_voiceEdit;
    QPlainTextEdit* m_fragmentEdit;
};

} // namespace openck

#endif // INFODATAWIDGET_HPP