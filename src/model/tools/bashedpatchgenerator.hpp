#ifndef BASHEDPATCHGENERATOR_HPP
#define BASHEDPATCHGENERATOR_HPP

#include <QObject>
#include <QString>
#include <QVector>

class Data;

class BashedPatchGenerator
{
public:
    struct PatchConfig
    {
        bool mergeNPCs = true;
        bool mergeWeapons = true;
        bool mergeArmor = true;
        bool mergeSpells = true;
        bool mergeAlchemy = true;
        bool mergeIngredients = true;
        bool mergeBooks = true;
        bool mergeEnchantments = true;
        bool mergeContainers = true;
        bool mergeMisc = true;
        bool mergeActivators = true;
        bool mergeRace = true;
        bool mergeClass = true;
        bool mergeQuest = true;
        bool mergePackage = true;
        bool mergeFact = true;
        bool mergePerk = true;
    };

    BashedPatchGenerator(Data* data);

    bool generatePatch(const QString& outputPath, const PatchConfig& config);
    QVector<QString> getMergedPluginList() const;
    int getMergedRecordCount() const;
    QString getPatchLog() const;

private:
    Data* mData;
    QVector<QString> mergedPlugins;
    int mergedRecordCount = 0;
    QString patchLog;

    template<typename RecordT, typename CollectionT>
    int mergeCollection(const QString& typeName, CollectionT& collection,
                        QVector<RecordT>& outputRecords);
};

#endif // BASHEDPATCHGENERATOR_HPP
