#ifndef DATAEXPORTER_H
#define DATAEXPORTER_H

#include "../../src/model/world/data.hpp"
#include "../../src/model/world/ckid.hpp"
#include "../../src/model/world/collection.hpp"
#include "../../src/model/world/record.hpp"

#include <QString>
#include <QList>
#include <QMap>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QDomDocument>
#include <QRegularExpression>

class DataExporter
{
public:
    enum class Format { JSON, CSV, XML };
    
    struct ExportFilter {
        QString editorIdPattern;
        quint32 formIdMin;
        quint32 formIdMax;
        bool onlyModified;
        bool onlyDeleted;
        ExportFilter() : formIdMin(0), formIdMax(0xFFFFFFFF), onlyModified(true), onlyDeleted(false) {}
    };
    
    struct ExportResult {
        int recordsExported;
        QString outputPath;
        QString error;
        quint64 fileSize;
        QMap<QString, int> recordsByType;
        ExportResult() : recordsExported(0), fileSize(0) {}
    };
    
    /// \brief Export records to JSON format
    /// \param data Source data containing all collections
    /// \param recordTypes List of record type names to export (e.g., "NPC_", "WEAP")
    /// \param outputPath Path for the output JSON file
    /// \param filter Optional filter for selecting specific records
    /// \return ExportResult with count, path, and any errors
    static ExportResult exportToJSON(const Data& data, const QStringList& recordTypes, const QString& outputPath, const ExportFilter& filter = ExportFilter());
    
    /// \brief Export records to CSV format
    /// \param data Source data containing all collections
    /// \param recordTypes List of record type names to export
    /// \param outputPath Path for the output CSV file
    /// \param filter Optional filter for selecting specific records
    /// \return ExportResult with count, path, and any errors
    static ExportResult exportToCSV(const Data& data, const QStringList& recordTypes, const QString& outputPath, const ExportFilter& filter = ExportFilter());
    
    /// \brief Export records to XML format
    /// \param data Source data containing all collections
    /// \param recordTypes List of record type names to export
    /// \param outputPath Path for the output XML file
    /// \param filter Optional filter for selecting specific records
    /// \return ExportResult with count, path, and any errors
    static ExportResult exportToXML(const Data& data, const QStringList& recordTypes, const QString& outputPath, const ExportFilter& filter = ExportFilter());
    
    /// \brief Export a single collection to JSON array
    /// \param collection Collection to export
    /// \param type Record type identifier
    /// \param array Output JSON array to populate
    /// \param filter Optional filter for selecting specific records
    /// \return Number of records exported
    static int exportCollectionToJSON(const BaseCollection& collection, CkId::Type type, QJsonArray& array, const ExportFilter& filter);
    
    /// \brief Export a single collection to CSV string
    /// \param collection Collection to export
    /// \param type Record type identifier
    /// \param csvContent Output CSV content string
    /// \param headers Output column headers
    /// \param filter Optional filter for selecting specific records
    /// \return Number of records exported
    static int exportCollectionToCSV(const BaseCollection& collection, CkId::Type type, QString& csvContent, QStringList& headers, const ExportFilter& filter);
    
    /// \brief Export a single collection to XML document
    /// \param collection Collection to export
    /// \param type Record type identifier
    /// \param doc Output QDomDocument to populate
    /// \param root Output root element reference
    /// \param filter Optional filter for selecting specific records
    /// \return Number of records exported
    static int exportCollectionToXML(const BaseCollection& collection, CkId::Type type, QDomDocument& doc, QDomElement& root, const ExportFilter& filter);
    
    /// \brief Convert a single record to JSON object
    /// \param record Record to convert
    /// \param type Record type identifier
    /// \return QJsonObject with all record fields serialized
    static QJsonObject recordToJSON(const BaseRecord& record, CkId::Type type);
    
    /// \brief Convert a single record to CSV field list
    /// \param record Record to convert
    /// \param type Record type identifier
    /// \param headers Output column headers (populated on first call)
    /// \return QStringList of field values matching headers
    static QStringList recordToCSVFields(const BaseRecord& record, CkId::Type type, QStringList& headers);
    
    /// \brief Convert a single record to XML element
    /// \param record Record to convert
    /// \param type Record type identifier
    /// \param doc QDomDocument to create element in
    /// \return QDomElement representing the record
    static QDomElement recordToXML(const BaseRecord& record, CkId::Type type, QDomDocument& doc);
    
    /// \brief Convert State enum to human-readable string
    /// \param state State value to convert
    /// \return String representation ("Base", "Modified", "ModifiedOnly", "Deleted")
    static QString getStateString(State state);

private:
    static QString getEditorId(const BaseRecord& record);
    static quint32 getFormId(const BaseRecord& record);
    static bool matchesFilter(const BaseRecord& record, const ExportFilter& filter);
};

#endif // DATAEXPORTER_H
