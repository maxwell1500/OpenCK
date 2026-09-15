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

#include <cstring>

namespace {

// Columns: Name, X, Y, Z, RotX, RotY, RotZ, FormID, Payload size. The shipped
// files always carry 0 or 24 payload bytes; a 0-byte entry simply has no
// transform (its cells stay blank and re-serialize without one).
constexpr int kColumnCount = 9;

QString floatText(float value)
{
    return QString::number(static_cast<double>(value), 'g', 9);
}

} // namespace

OpalPlacementDialog::OpalPlacementDialog(QWidget* parent)
    : QDialog(parent)
{
    LOG_INFO("OpalPlacementDialog: opening OPAL placement editor");

    setWindowTitle(tr("OPAL Placement Editor"));
    resize(820, 480);

    auto* layout = new QVBoxLayout(this);

    m_table = new QTableWidget(0, kColumnCount, this);
    m_table->setHorizontalHeaderLabels({
        tr("Name"), tr("X"), tr("Y"), tr("Z"),
        tr("RotX"), tr("RotY"), tr("RotZ"), tr("FormID"), tr("Payload")});
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
    connect(saveBtn, &QPushButton::clicked, this, &OpalPlacementDialog::saveFromFile);

    rebuildTable();
}

void OpalPlacementDialog::setList(const OpalList& list)
{
    m_list = list;
    rebuildTable();
}

void OpalPlacementDialog::rebuildTable()
{
    m_table->clearContents();
    m_table->setRowCount(m_list.placements.size());

    for (int r = 0; r < m_list.placements.size(); ++r)
    {
        const OpalPlacement& p = m_list.placements[r];
        m_table->setItem(r, 0, new QTableWidgetItem(p.name));

        const QVector<float> t = p.transform();
        if (p.hasTransform())
        {
            for (int i = 0; i < 6; ++i)
                m_table->setItem(r, 1 + i, new QTableWidgetItem(floatText(t[i])));
        }

        m_table->setItem(r, 7, new QTableWidgetItem(
            QStringLiteral("%1").arg(p.formId(), 8, 16, QLatin1Char('0'))));
        m_table->setItem(r, 8, new QTableWidgetItem(
            QString::number(p.payload.size())));
    }
}

OpalList OpalPlacementDialog::list() const
{
    OpalList out;
    out.version = m_list.version;

    for (int r = 0; r < m_table->rowCount(); ++r)
    {
        OpalPlacement p;
        auto* name = m_table->item(r, 0);
        p.name = name ? name->text() : QString();

        // A row carries a transform when the original had one (payload size
        // cell) or when any transform cell is filled. Preserve the original
        // payload otherwise so 0-byte entries stay 0 bytes.
        const bool originalHadTransform =
            r < m_list.placements.size() && m_list.placements[r].hasTransform();
        bool anyTransform = false;
        for (int i = 1; i <= 6; ++i)
        {
            auto* item = m_table->item(r, i);
            if (item && !item->text().trimmed().isEmpty())
                anyTransform = true;
        }
        if (originalHadTransform || anyTransform)
        {
            p.payload.resize(24);
            for (int i = 0; i < 6; ++i)
            {
                auto* item = m_table->item(r, 1 + i);
                const float value = item ? item->text().toFloat() : 0.0f;
                quint32 bits = 0;
                std::memcpy(&bits, &value, sizeof(bits));
                p.payload[i * 4 + 0] = static_cast<char>(bits & 0xFF);
                p.payload[i * 4 + 1] = static_cast<char>((bits >> 8) & 0xFF);
                p.payload[i * 4 + 2] = static_cast<char>((bits >> 16) & 0xFF);
                p.payload[i * 4 + 3] = static_cast<char>((bits >> 24) & 0xFF);
            }
        }

        auto* formId = m_table->item(r, 7);
        const quint32 id = formId ? formId->text().toUInt(nullptr, 16) : 0u;
        p.trailer = id;
        out.placements.append(p);
    }
    return out;
}

void OpalPlacementDialog::addRow()
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    for (int c = 0; c < kColumnCount; ++c)
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
            tr("Could not read %1 (not a valid .opl list).").arg(path));
        return;
    }
    setList(loaded);
}

void OpalPlacementDialog::saveFromFile()
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
