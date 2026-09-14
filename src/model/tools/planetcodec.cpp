#include "planetcodec.hpp"

#include "../../../libs/files/esm/common.hpp"
#include "../../../libs/files/esm/pndrecord.hpp"

namespace PlanetCodec
{

namespace {

// Observed shipped defaults for placeholder orbital PNDT records.
constexpr float kDefaultDensity = 1.0f;
constexpr float kDefaultPhase = 1.0f;
constexpr quint32 kDefaultResources = 0;

float temperatureFromString(const QString& s)
{
    bool ok = false;
    const double v = s.trimmed().toDouble(&ok);
    return ok ? static_cast<float>(v) : 0.0f;
}

} // namespace

PndRecord toRecord(const PlanetDefinition& def, const PndRecord* base)
{
    PndRecord rec;
    if (base)
        rec = *base;
    else
        rec.blank();

    rec.editorId = def.editorId;
    rec.starSystem = def.starSystem;

    // The temperature string doubles as the numeric carrier: fromRecord()
    // writes the measured float here, the editor may replace it with a label.
    // A label over a base record keeps the base's measured TEMP.
    bool numeric = false;
    def.temperature.trimmed().toDouble(&numeric);
    if (!base || numeric)
        rec.temperature = temperatureFromString(def.temperature);

    if (!base)
    {
        rec.density = kDefaultDensity;
        rec.phase = kDefaultPhase;
        rec.resources = kDefaultResources;
        // A new record names its typed subrecords so save() emits them.
        rec.mOrder.append((NAME)'EDID');
        rec.mOrder.append((NAME)'FNAM');
        rec.mOrder.append((NAME)'ANAM');
        rec.mOrder.append((NAME)'TEMP');
        rec.mOrder.append((NAME)'DENS');
        rec.mOrder.append((NAME)'PHLA');
        rec.mOrder.append((NAME)'RSCS');
    }
    return rec;
}

PlanetDefinition fromRecord(const PndRecord& rec)
{
    PlanetDefinition def;
    def.editorId = rec.editorId;
    def.starSystem = rec.starSystem;
    def.temperature = QString::number(static_cast<double>(rec.temperature));
    return def;
}

bool hasUndecodedData(const PndRecord& rec)
{
    return !rec.rawSubRecords.isEmpty();
}

} // namespace PlanetCodec
