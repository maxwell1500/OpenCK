#include "opalplacementdialog.hpp"

#include "../../libs/files/log/logger.hpp"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

OpalPlacementDialog::OpalPlacementDialog(QWidget* parent)
    : QDialog(parent)
{
    LOG_INFO("OpalPlacementDialog: opening OPAL placement editor");

    setWindowTitle(tr("OPAL Placement Editor"));
    resize(640, 480);

    auto* layout = new QVBoxLayout(this);

    m_table = new QTableWidget(0, 0, this);
    m_table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_table);

    auto* rowButtons = new QHBoxLayout();
    auto* addBtn = new QPushButton(tr("Add Row"), this);
    auto* removeBtn = new QPushButton(tr("Remove Row"), this);
    rowButtons->addWidget(addBtn);
    rowButtons->addWidget(removeBtn);
    rowButtons->addStretch();
    layout->addLayout(rowButtons);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* loadBtn = buttons->addButton(tr("Load .opl..."), QDialogButtonBox::ActionRole);
    auto* saveBtn = buttons->addButton(tr("Save .opl..."), QDialogButtonBox::ActionRole);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        m_list = list();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(addBtn, &QPushButton::clicked, this, &OpalPlacementDialog::addRow);
    connect(removeBtn, &QPushButton::clicked, this, &OpalPlacementDialog::removeRow);
    connect(loadBtn, &QPushButton::clicked, this, &OpalPlacementDialog::loadFromFile);
    connect(saveBtn, &QPushButton::clicked, this, &OpalPlacementDialog::saveToFile);

    rebuildTable();
}

void OpalPlacementDialog::setList(const OpalList& list)
{
    m_list = list;
    rebuildTable();
}

void OpalPlacementDialog::rebuildTable()
{
    m_table->clear();
    m_table->setColumnCount(m_list.headers.size());
    m_table->setHorizontalHeaderLabels(m_list.headers);
    m_table->setRowCount(m_list.rows.size());

    for (int r = 0; r < m_list.rows.size(); ++r)
    {
        for (int c = 0; c < m_list.headers.size(); ++c)
        {
            const QString value = c < m_list.rows[r].size() ? m_list.rows[r][c] : QString();
            m_table->setItem(r, c, new QTableWidgetItem(value));
        }
    }
}

OpalList OpalPlacementDialog::list() const
{
    OpalList out;
    out.headers = m_list.headers;

    for (int r = 0; r < m_table->rowCount(); ++r)
    {
        QVector<QString> row;
        for (int c = 0; c < m_table->columnCount(); ++c)
        {
            auto* item = m_table->item(r, c);
            row.append(item ? item->text() : QString());
        }
        out.rows.append(row);
    }
    return out;
}

void OpalPlacementDialog::addRow()
{
    const int colCount = qMax(1, m_table->columnCount());
    if (m_table->columnCount() == 0)
        m_table->setColumnCount(colCount);

    const int row = m_table->rowCount();
    m_table->insertRow(row);
    for (int c = 0; c < m_table->columnCount(); ++c)
        m_table->setItem(row, c, new QTableWidgetItem(QString()));
}

void OpalPlacementDialog::removeRow()
{
    const int row = m_table->currentRow();
    if (row >= 0)
        m_table->removeRow(row);
}

void OpalPlacementDialog::loadFromFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Load OPAL Placement List"), QString(),
        tr("OPAL lists (*.opl);;All files (*)"));
    if (path.isEmpty())
        return;

    OpalList loaded;
    if (!OpalList::loadFile(path, loaded))
    {
        QMessageBox::warning(this, tr("Load Failed"),
            tr("Could not read %1").arg(path));
        return;
    }
    setList(loaded);
}

void OpalPlacementDialog::saveToFile()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save OPAL Placement List"), QStringLiteral("placement.opl"),
        tr("OPAL lists (*.opl);;All files (*)"));
    if (path.isEmpty())
        return;

    if (!list().saveFile(path))
    {
        QMessageBox::warning(this, tr("Save Failed"),
            tr("Could not write %1").arg(path));
        return;
    }
}
