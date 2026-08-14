#include "datatable.hpp"

#include "logger.hpp"

#include <QBrush>
#include <QDir>

DataTable::DataTable(const QString& path, QObject* parent)
    : QAbstractTableModel(parent),
      active(NONE_ACTIVE)
{
    LOG_INFO(QString("DataTable::DataTable starting with path='%1'").arg(path));
    QDir dataDir{ path };

    QStringList filters{ "*.esm", "*.esp", "*.esl" };
    dataDir.setNameFilters(filters);
    QStringList files{ dataDir.entryList() };
    LOG_INFO(QString("DataTable: entryList found %1 files").arg(files.size()));

    if (!dataDir.exists() || files.empty())
    {
        loadErrors.append(QString("No data files found in '%1'.").arg(path));
        LOG_WARNING(QString("DataTable: no data files in '%1'").arg(path));
    }

    // NOTE: Files are sorted alphabetically. Plugin/master load order is
    // determined by the application layer if needed; here we just ensure
    // a deterministic ordering.
    files.sort();

    int idx = 0;
    for (auto file: files)
    {
        try
        {
            QString fileName{ path + "/" + file };
            LOG_INFO(QString("DataTable: opening '%1' (%2/%3)").arg(file).arg(idx + 1).arg(files.size()));
            ESMReader reader{ fileName };
            reader.open();
            FileInfo info = getFileInfo(file, reader.getHeader());
            filesInfo.push_back(info);
            selected.push_back(false);
            LOG_INFO(QString("DataTable: parsed '%1' OK").arg(file));
        }
        catch (std::runtime_error& e)
        {
            loadErrors.append(QString("%1: %2").arg(file, e.what()));
            LOG_WARNING(QString("DataTable: failed to read '%1': %2").arg(file, e.what()));
        }
        ++idx;
    }
    LOG_INFO(QString("DataTable::DataTable complete, %1 files loaded, %2 errors").arg(filesInfo.size()).arg(loadErrors.size()));
}

int DataTable::rowCount(const QModelIndex &parent) const
{
    return filesInfo.size();
}

int DataTable::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant DataTable::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case 0:
            return filesInfo.at(index.row()).fileName;
        case 1:
        {
            if (index.row() == active)
            {
                return "Active file";
            }
            else if (filesInfo.at(index.row()).fileName.endsWith(".esm", Qt::CaseInsensitive))
            {
                return "Master File";
            }
            else if (filesInfo.at(index.row()).fileName.endsWith(".esl", Qt::CaseInsensitive))
            {
                return "Light Master";
            }
            else
            {
                return "Plugin File";
            }
        }
        }
    }
    else if (role == Qt::CheckStateRole && index.column() == 0)
    {
        return QVariant::fromValue(
            selected.at(index.row()) ? Qt::Checked : Qt::Unchecked
        );
    }

    return QVariant();
}

bool DataTable::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::CheckStateRole && index.column() == 0)
    {
        selected.replace(index.row(), value.toBool());
        QModelIndex topLeft(this->index(0, 0));
        QModelIndex bottomRight(this->index(rowCount(), columnCount()));
        emit dataChanged(topLeft, bottomRight);
        return true;
    }
    return QAbstractTableModel::setData(index, value, role);
}

void DataTable::doubleClicked(const QModelIndex& indx)
{
    if (active == indx.row())
    {
        selected.replace(indx.row(), false);
        active = NONE_ACTIVE;
    }
    else
    {
        bool val = selected.at(indx.row());
        selected.replace(indx.row(), !val);
    }

    QModelIndex topLeft(index(0, 0));
    QModelIndex bottomRight(index(rowCount(), columnCount()));
    emit dataChanged(topLeft, bottomRight);
}

void DataTable::setActive(const QModelIndex& indx)
{
    active = indx.row();
    selected.replace(indx.row(), true);

    QModelIndex topLeft(index(0, 0));
    QModelIndex bottomRight(index(rowCount(), columnCount()));
    emit dataChanged(topLeft, bottomRight);
}

bool DataTable::isPlugin(const QModelIndex& index) const
{
    return isPlugin(index.row());
}

bool DataTable::isPlugin(int row) const
{
    return (!filesInfo.at(row).flags.test(FileFlag::Master) &&
            !filesInfo.at(row).flags.test(FileFlag::LightMaster));
}

Qt::ItemFlags DataTable::flags(const QModelIndex& index) const
{
    Qt::ItemFlags baseFlags {
        Qt::ItemFlag::ItemIsEnabled |
        Qt::ItemFlag::ItemIsSelectable
    };

    switch (index.column())
    {
    case 0:
        return baseFlags | Qt::ItemFlag::ItemIsUserCheckable;
    }

    return baseFlags;
}

QVariant DataTable::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return QString("TES File");
        case 1:
            return QString("Status");
        }
    }
    else
    {
        switch (role)
        {
        case Qt::TextAlignmentRole:
            return Qt::AlignLeft;
        }
    }

    return QVariant();
}

FileInfo DataTable::getInfoAtSelected(const QModelIndex &selected)
{
    FileInfo info{ filesInfo.at(selected.row()) };
    emit newFileSelected(info);
    return info;
}

FileInfo DataTable::getFileInfo(QString fileName, Header header)
{
    FileInfo info;
    info.fileName = fileName;
    info.author = header.author;
    info.description = header.description;
    info.flags.val = header.flags.val;
    info.version = header.version;

    for (auto master: header.masters)
    {
        info.masters << master.name;
    }

    return info;
}

void DataTable::setSelectedFiles(const QStringList& files)
{
    for (int i = 0; i < filesInfo.size(); i++)
    {
        if (files.contains(filesInfo.at(i).fileName))
        {
            selected.replace(i, true);
        }
    }

    QModelIndex topLeft(index(0, 0));
    QModelIndex bottomRight(index(rowCount(), columnCount()));
    emit dataChanged(topLeft, bottomRight);
}

std::tuple<QStringList, int> DataTable::getFiles() const
{
    int active_ = -1;
    QStringList files;

    for (int i = 0; i < filesInfo.size(); i++)
    {
        if (selected.at(i))
        {
            if (!files.contains(filesInfo.at(i).fileName))
            {
                files << filesInfo.at(i).fileName;

                if (i == active)
                {
                    active_ = files.size() - 1;
                }
            }

            for (const auto& master : filesInfo.at(i).masters)
            {
                if (!files.contains(master))
                {
                    files << master;
                }
            }
        }
    }

    // The loader treats the last file as the edited plugin (masters before
    // it are indexed/deferred). Move the active file to the end so opening
    // a plugin with its masters loads the plugin eagerly, not whichever
    // master happened to be appended last.
    if (active_ >= 0 && active_ != files.size() - 1)
    {
        const QString activeName = files.at(active_);
        files.removeAt(active_);
        files << activeName;
        active_ = files.size() - 1;
    }

    return std::make_tuple(files, active_);
}

int DataTable::getActiveRow() const
{
    return active;
}