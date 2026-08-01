#include "scriptmanagerdialog.hpp"

#include "logger.hpp"

#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QFile>
#include <QMessageBox>

ScriptManagerDialog::ScriptManagerDialog(const QString& scriptsRoot, QWidget* parent)
    : QDialog(parent)
    , m_scriptsRoot(scriptsRoot)
    , m_list(nullptr)
    , m_filterEdit(nullptr)
    , m_openBtn(nullptr)
    , m_newBtn(nullptr)
{
    setWindowTitle("Papyrus Script Manager");
    resize(420, 520);

    auto* layout = new QVBoxLayout(this);

    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel(tr("Filter:")));
    m_filterEdit = new QLineEdit();
    m_filterEdit->setPlaceholderText(tr("Filter by script name..."));
    filterRow->addWidget(m_filterEdit, 1);
    layout->addLayout(filterRow);

    m_list = new QListWidget();
    layout->addWidget(m_list, 1);

    auto* btnRow = new QHBoxLayout();
    m_newBtn = new QPushButton(tr("New Script..."));
    m_openBtn = new QPushButton(tr("Open"));
    m_openBtn->setDefault(true);
    btnRow->addWidget(m_newBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_openBtn);
    layout->addLayout(btnRow);

    connect(m_filterEdit, &QLineEdit::textChanged, this, &ScriptManagerDialog::onFilterChanged);
    connect(m_openBtn, &QPushButton::clicked, this, &ScriptManagerDialog::onOpen);
    connect(m_newBtn, &QPushButton::clicked, this, &ScriptManagerDialog::onNewScript);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &ScriptManagerDialog::onDoubleClick);

    populate();
}

void ScriptManagerDialog::populate()
{
    m_allScripts.clear();
    m_list->clear();

    const QDir root(m_scriptsRoot);
    if (!root.exists())
    {
        LOG_WARNING(QString("ScriptManager: scripts root does not exist: %1").arg(m_scriptsRoot));
        return;
    }

    const QFileInfoList files = root.entryInfoList(
        { QStringLiteral("*.psc") }, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& fi : files)
    {
        m_allScripts.append(fi.absoluteFilePath());
    }

    // Also scan immediate subfolders (Source/Base, etc.).
    const QFileInfoList dirs = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& d : dirs)
    {
        const QFileInfoList sub = QDir(d.absoluteFilePath()).entryInfoList(
            { QStringLiteral("*.psc") }, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo& fi : sub)
        {
            m_allScripts.append(fi.absoluteFilePath());
        }
    }

    onFilterChanged(m_filterEdit->text());
}

void ScriptManagerDialog::onFilterChanged(const QString& text)
{
    m_list->clear();
    const QString needle = text.trimmed();
    for (const QString& path : m_allScripts)
    {
        const QString name = QFileInfo(path).fileName();
        if (needle.isEmpty() || name.contains(needle, Qt::CaseInsensitive))
        {
            m_list->addItem(name);
        }
    }
}

QString ScriptManagerDialog::selectedScript() const
{
    const QListWidgetItem* item = m_list->currentItem();
    if (!item) return QString();
    const QString name = item->text();
    for (const QString& path : m_allScripts)
    {
        if (QFileInfo(path).fileName() == name)
        {
            return path;
        }
    }
    return QString();
}

void ScriptManagerDialog::onOpen()
{
    const QString path = selectedScript();
    if (path.isEmpty())
    {
        QMessageBox::information(this, tr("Open Script"), tr("Select a script first."));
        return;
    }
    accept();
}

void ScriptManagerDialog::onDoubleClick()
{
    if (m_list->currentItem())
        accept();
}

void ScriptManagerDialog::onNewScript()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("New Script"),
        tr("Script name (without .psc):"), QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;

    const QString cleanName = name.trimmed();
    const QDir root(m_scriptsRoot);
    if (!root.exists())
    {
        QMessageBox::warning(this, tr("New Script"),
            tr("Scripts root does not exist:\n%1").arg(m_scriptsRoot));
        return;
    }
    const QString path = root.filePath(cleanName + QStringLiteral(".psc"));
    if (QFile::exists(path))
    {
        QMessageBox::warning(this, tr("New Script"), tr("Script already exists."));
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, tr("New Script"), tr("Could not create script."));
        return;
    }
    file.write("ScriptName " + cleanName.toUtf8() + "\n\n");
    file.close();

    m_allScripts.append(path);
    onFilterChanged(m_filterEdit->text());
    LOG_INFO(QString("ScriptManager: created %1").arg(path));
    accept();
}
