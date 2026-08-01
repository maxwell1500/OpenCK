#ifndef PARTICLESLODPRESETS_HPP
#define PARTICLESLODPRESETS_HPP

#include <QString>
#include <QVector>

// Parses particle LOD presets (ParticlesLODPresets.json). The real file
// defines per-category particle budgets for Near/Middle/Far distances.
struct ParticleLodPresets
{
    struct Category
    {
        QString name;
        int nearBudget = 0;
        int middleBudget = 0;
        int farBudget = 0;
    };

    // Parses JSON of the form:
    //   { "presets": [ { "category": "Fire", "near": 100,
    //                    "middle": 50, "far": 10 }, ... ] }
    // Unknown fields are ignored. Returns parsed presets.
    static QVector<Category> parseJson(const QByteArray& json);

    // Loads and parses the given file. Returns false if unreadable.
    static bool loadFile(const QString& path, QVector<Category>& out);
};

#endif // PARTICLESLODPRESETS_HPP
