#ifndef SEARCHDIALOG_HPP
#define SEARCHDIALOG_HPP

#include "../../model/tools/searchalgorithm.hpp"

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QStringList>

class Data;

class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    SearchDialog(Data* data, QWidget* parent = nullptr);
    ~SearchDialog();

private slots:
    void onSearch();
    void onDoubleClicked(QListWidgetItem* item);
    void onCopyFormId();
    void onHistoryActivated(int index);
    void onSaveSearch();
    void onLoadSearch();
    void onAddCriterion();
    void onRemoveCriterion(int index);

private:
    void setupUI();
    void populateResults(const QVector<SearchAlgorithm::SearchResult>& results);
    QString formatResultText(const SearchAlgorithm::SearchResult& result) const;
    void openRecordEditor(const SearchAlgorithm::SearchResult& result);
    void cloneSelected();
    void deleteSelected();

    // Batch Editing (Step 2)
    bool enableBatchEditing(bool enabled = true);
    void batchSetEditorIdForSelected();
    void batchCloneSelected();
    QList<int> getSelectedIndices() const;

    // Regex support
    SearchAlgorithm::MatchMode getCurrentMatchMode() const;

    // Search history
    void loadHistory();
    void saveHistory();
    void addToHistory(const QString& text);
    void updateHistoryCombo();

    // Saved searches
    void loadSavedSearches();
    QVariantMap buildCurrentCriteriaMap() const;
    void applyCriteriaMap(const QVariantMap& map);

    // Multi-criteria
    void addCriterionRow(const QString& field = "", const QString& mode = "Contains", const QString& text = "");
    QWidget* createCriterionRow(int index);
    void rebuildCriteriaLayout();

    Data* mData;
    QLineEdit* mSearchEdit;
    QPushButton* mFilterButton;
    QComboBox* mTypeCombo;
    QComboBox* mFieldCombo;
    QListWidget* mResultsList;
    QPushButton* mEditButton;
    QPushButton* mCloneButton;
    QPushButton* mDeleteButton;
    QPushButton* mCloseButton;
    QLabel* mStatusLabel;

    // Regex toggle
    QCheckBox* mRegexCheckBox;

    // Search history
    QComboBox* mHistoryCombo;
    QStringList mSearchHistory;

    // Saved searches
    QPushButton* mSaveSearchBtn;
    QPushButton* mLoadSearchBtn;

    // Multi-criteria
    QVBoxLayout* mCriteriaLayout;
    QPushButton* mAddCriterionBtn;
    QComboBox* mLogicCombo;
    QVector<QWidget*> mCriteriaRows;

    QVector<SearchAlgorithm::SearchResult> mResults;
};

#endif // SEARCHDIALOG_HPP
