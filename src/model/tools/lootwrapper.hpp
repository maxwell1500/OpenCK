#ifndef LOOTWRAPPER_HPP
#define LOOTWRAPPER_HPP

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

class LootWrapper : public QObject
{
    Q_OBJECT

public:
    explicit LootWrapper(QObject* parent = nullptr);
    ~LootWrapper();

    bool isAvailable();
    QString findLootPath();
    QVector<QString> sortPlugins(const QVector<QString>& plugins);
    QStringList getMasterPlugins(const QString& pluginPath);

    void setLootPath(const QString& path);
    QString getLootPath() const;

private:
    QString m_lootPath;
    bool m_checked;
};

#endif // LOOTWRAPPER_HPP
