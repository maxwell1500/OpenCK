#ifndef QUICKFILTERBAR_H
#define QUICKFILTERBAR_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class QuickFilterBar : public QWidget
{
    Q_OBJECT

public:
    explicit QuickFilterBar(QWidget* parent = nullptr);

    void setPlaceholderText(const QString& text);
    void setFilterFields(const QStringList& fields);
    void setMatchModes(const QStringList& modes);
    
    QString getText() const;
    QString getField() const;
    QString getMatchMode() const;
    bool isRegexEnabled() const;
    void setResultCount(int count);

signals:
    void filterChanged(const QString& text);
    void filterCleared();
    void searchClicked();

public slots:
    void clearFilter();
    void setText(const QString& text);

private slots:
    void onTextchanged();
    void onSearchClicked();

private:
    QLineEdit* mSearchEdit;
    QComboBox* mFieldCombo;
    QComboBox* mMatchModeCombo;
    QPushButton* mRegexButton;
    QPushButton* mClearButton;
    QLabel* mResultCount;
    
    bool mRegexEnabled;
    QTimer* mDebounceTimer;
};

#endif // QUICKFILTERBAR_H
