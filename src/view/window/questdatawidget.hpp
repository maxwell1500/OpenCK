#ifndef QUESTDATAWIDGET_HPP
#define QUESTDATAWIDGET_HPP

#include <QWidget>

class QTableWidget;
class QSpinBox;
class QPlainTextEdit;
class QLineEdit;
class QCheckBox;
class QLabel;

namespace openck {

class FormComponents;

class QuestDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QuestDataWidget(void* recordPtr, FormComponents* components,
                             QWidget* parent = nullptr);
    ~QuestDataWidget() override;

private slots:
    void onAddStage();
    void onRemoveStage();
    void onStageSelectionChanged();
    void onStageCellChanged(int row, int column);
    void onDetailChanged();
    void onCompileFragment();

private:
    void populateStageTable();
    void loadStageDetail(int row);
    void clearDetailPanel();
    void syncDetailToRecord();
    void syncFragmentToRaw();

    void* m_recordPtr;
    QTableWidget* m_stageTable;
    QSpinBox* m_indexSpin;
    QCheckBox* m_doneFlagCheck;
    QCheckBox* m_repeatFlagCheck;
    QPlainTextEdit* m_descEdit;
    QLineEdit* m_objectiveEdit;
    QPlainTextEdit* m_fragmentEdit;
    QLabel* m_statusLabel;
    int m_selectedRow;
    bool m_syncing;
};

} // namespace openck

#endif // QUESTDATAWIDGET_HPP