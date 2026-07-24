#include "quickfilterbar.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QStyle>

QuickFilterBar::QuickFilterBar(QWidget* parent)
    : QWidget(parent),
      mRegexEnabled(false)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);
    
    mSearchEdit = new QLineEdit(this);
    mSearchEdit->setPlaceholderText("Filter records...");
    mSearchEdit->setMinimumWidth(200);
    layout->addWidget(mSearchEdit, 1);
    
    mFieldCombo = new QComboBox(this);
    mFieldCombo->addItems({"All Fields", "EditorID", "FormID", "Name"});
    mFieldCombo->setMinimumWidth(100);
    layout->addWidget(mFieldCombo);
    
    mMatchModeCombo = new QComboBox(this);
    mMatchModeCombo->addItems({"Contains", "Starts With", "Ends With", "Exact", "Regex"});
    mMatchModeCombo->setMinimumWidth(100);
    layout->addWidget(mMatchModeCombo);
    
    mRegexButton = new QPushButton(this);
    mRegexButton->setText(".*");
    mRegexButton->setToolTip("Toggle regex mode");
    mRegexButton->setCheckable(true);
    layout->addWidget(mRegexButton);
    
    mClearButton = new QPushButton(this);
    mClearButton->setText("\u00D7");
    mClearButton->setToolTip("Clear filter");
    mClearButton->setMinimumWidth(28);
    layout->addWidget(mClearButton);
    
    mResultCount = new QLabel(this);
    mResultCount->setMinimumWidth(80);
    mResultCount->setText("0 results");
    mResultCount->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(mResultCount);
    
    connect(mSearchEdit, &QLineEdit::textChanged, this, &QuickFilterBar::onTextchanged);
    connect(mSearchEdit, &QLineEdit::returnPressed, this, &QuickFilterBar::onSearchClicked);
    connect(mClearButton, &QPushButton::clicked, this, &QuickFilterBar::clearFilter);
    connect(mRegexButton, &QPushButton::toggled, this, [this](bool checked) {
        mRegexEnabled = checked;
        if (checked)
        {
            mMatchModeCombo->setCurrentText("Regex");
        }
    });
    
    mDebounceTimer = new QTimer(this);
    mDebounceTimer->setSingleShot(true);
    mDebounceTimer->setInterval(300);
    connect(mDebounceTimer, &QTimer::timeout, this, [this]() {
        emit filterChanged(mSearchEdit->text());
    });
}

void QuickFilterBar::setPlaceholderText(const QString& text)
{
    mSearchEdit->setPlaceholderText(text);
}

void QuickFilterBar::setFilterFields(const QStringList& fields)
{
    mFieldCombo->clear();
    mFieldCombo->addItems(fields);
}

void QuickFilterBar::setMatchModes(const QStringList& modes)
{
    mMatchModeCombo->clear();
    mMatchModeCombo->addItems(modes);
}

QString QuickFilterBar::getText() const
{
    return mSearchEdit->text();
}

QString QuickFilterBar::getField() const
{
    return mFieldCombo->currentText();
}

QString QuickFilterBar::getMatchMode() const
{
    return mMatchModeCombo->currentText();
}

bool QuickFilterBar::isRegexEnabled() const
{
    return mRegexEnabled;
}

void QuickFilterBar::clearFilter()
{
    mSearchEdit->clear();
    mFieldCombo->setCurrentText("All Fields");
    mMatchModeCombo->setCurrentText("Contains");
    mRegexEnabled = false;
    mRegexButton->setChecked(false);
    mResultCount->setText("0 results");
    emit filterCleared();
}

void QuickFilterBar::setText(const QString& text)
{
    mSearchEdit->setText(text);
}

void QuickFilterBar::onTextchanged()
{
    mDebounceTimer->start();
}

void QuickFilterBar::onSearchClicked()
{
    mDebounceTimer->stop();
    emit filterChanged(mSearchEdit->text());
}

void QuickFilterBar::setResultCount(int count)
{
    mResultCount->setText(QString("%1 result(s)").arg(count));
}
