#ifndef PLUGINTEXTEXPORT_HPP
#define PLUGINTEXTEXPORT_HPP

#include <QJsonObject>
#include <QString>
#include <QVector>

#include "../../../libs/files/esm/subrecordsnapshot.hpp"

// Phase 10.2: decompile a binary ESM/ESP into deterministic, sorted JSON
// text so a plugin can be tracked in Git with readable, mergeable diffs.
//
// The export is a pure function of the on-disk records: records are emitted
// in (type, formId) order, subrecords in file order, payloads as lowercase
// hex, and every timestamp/VC byte is normalized out. Byte-identical inputs
// therefore produce byte-identical output, which is what makes the files
// diffable and mergeable rather than a diff-eating blob.
namespace openck {

class PluginTextExport
{
public:
    /// Writes <output>.json for the plugin at path. Returns false and sets
    /// error when the plugin cannot be read or the file cannot be written.
    static bool exportJson(const QString& pluginPath, const QString& outputPath,
                           QString* error = nullptr);

    /// The JSON object for a plugin (records → subrecords → hex payload).
    static QJsonObject toJson(const QVector<RecordSnapshot>& records);
};

} // namespace openck

#endif // PLUGINTEXTEXPORT_HPP
