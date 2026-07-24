#include "pluginvalidator.hpp"
#include "esmreader.hpp"
#include "tes4.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QByteArray>
#include <QtEndian>

QVector<PluginValidator::Issue> PluginValidator::getMissingMasters(const QString& pluginPath, const QStringList& availablePlugins)
{
    QVector<Issue> issues;
    
    QFile file(pluginPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        Issue issue;
        issue.message = QString("Cannot open plugin file: %1").arg(pluginPath);
        issue.severity = Severity::Error;
        issue.category = "Structure";
        issues.append(issue);
        return issues;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    if (data.size() < 32)
    {
        Issue issue;
        issue.message = "File too small to be a valid plugin";
        issue.severity = Severity::Error;
        issue.category = "Structure";
        issues.append(issue);
        return issues;
    }
    
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    
    QByteArray magic;
    stream.readRawData(magic.data(), 4);
    
    if (magic != "TES4")
    {
        Issue issue;
        issue.message = "Invalid plugin format (missing TES4 header)";
        issue.severity = Severity::Error;
        issue.category = "Structure";
        issues.append(issue);
        return issues;
    }
    
    quint16 headerSize = 0;
    quint32 updateTimeStamp = 0;
    quint32 numRecords = 0;
    stream >> headerSize >> updateTimeStamp >> numRecords;
    
    quint16 numMasters = 0;
    stream >> numMasters;
    
    for (quint16 i = 0; i < numMasters; i++)
    {
        QByteArray masterData;
        stream.skipRawData(4); // skip size field
        
        QByteArray masterNameBytes;
        masterNameBytes.resize(256);
        stream.readRawData(masterNameBytes.data(), 256);
        
        int nullPos = masterNameBytes.indexOf('\0');
        if (nullPos >= 0)
        {
            masterNameBytes.truncate(nullPos);
        }
        
        QString masterName = QString::fromLatin1(masterNameBytes).trimmed();
        if (!masterName.isEmpty() && !availablePlugins.contains(masterName))
        {
            Issue issue;
            issue.message = QString("Missing master: %1").arg(masterName);
            issue.severity = Severity::Warning;
            issue.category = "Masters";
            issues.append(issue);
        }
    }
    
    return issues;
}

QVector<PluginValidator::Issue> PluginValidator::checkFormIdRanges(const QString& pluginPath)
{
    QVector<Issue> issues;
    
    QFile file(pluginPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return issues;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    
    QByteArray magic;
    magic.resize(4);
    stream.readRawData(magic.data(), 4);
    
    if (magic != "TES4")
    {
        return issues;
    }
    
    quint16 headerSize = 0;
    quint32 updateTimeStamp = 0;
    quint32 numRecords = 0;
    stream >> headerSize >> updateTimeStamp >> numRecords;
    
    quint16 numMasters = 0;
    stream >> numMasters;
    
    for (quint16 i = 0; i < numMasters; i++)
    {
        QByteArray sizeBytes;
        sizeBytes.resize(4);
        stream.readRawData(sizeBytes.data(), 4);
        
        QByteArray masterNameBytes;
        masterNameBytes.resize(256);
        stream.readRawData(masterNameBytes.data(), 256);
    }
    
    quint32 formId = 0;
    int recordCount = 0;
    qint64 streamPos = 0;
    
    while (streamPos < data.size() - 8)
    {
        QByteArray idBytes;
        idBytes.resize(4);
        stream.readRawData(idBytes.data(), 4);
        streamPos += 4;
        
        if (idBytes == "EDID" || idBytes == "CNAM" || idBytes == "SNAM" || 
            idBytes == "DNAM" || idBytes == "FNAM" || idBytes == "ANAM")
        {
            quint32 dataSize = 0;
            stream >> dataSize;
            stream.skipRawData(dataSize);
            streamPos += 4 + dataSize;
            continue;
        }
        
        formId = qFromLittleEndian<quint32>(idBytes.data());
        recordCount++;
        
        if ((formId & 0xFF000000) == 0)
        {
            Issue issue;
            issue.message = QString("Record %1 has FormID 0x%2 (no plugin index)").arg(recordCount).arg(formId, 8, 16);
            issue.severity = Severity::Warning;
            issue.category = "FormIDs";
            issues.append(issue);
        }
        
        QByteArray sizeBytes;
        sizeBytes.resize(4);
        stream.readRawData(sizeBytes.data(), 4);
        quint32 dataSize = qFromLittleEndian<quint32>(sizeBytes.data());
        stream.skipRawData(dataSize);
        streamPos += 4 + 4 + dataSize;
    }
    
    if (recordCount == 0)
    {
        Issue issue;
        issue.message = "Plugin contains no records";
        issue.severity = Severity::Info;
        issue.category = "Records";
        issues.append(issue);
    }
    
    return issues;
}

PluginValidator::PluginReport PluginValidator::validate(const QString& pluginPath, const QStringList& availablePlugins)
{
    PluginReport report;
    report.pluginName = QFileInfo(pluginPath).fileName();
    report.totalRecords = 0;
    report.modifiedRecords = 0;
    report.missingMasters = 0;
    
    auto missingMasters = getMissingMasters(pluginPath, availablePlugins);
    report.missingMasters = missingMasters.size();
    
    auto formIdIssues = checkFormIdRanges(pluginPath);
    for (auto& issue : formIdIssues)
    {
        report.issues.append(issue);
    }
    
    QFile file(pluginPath);
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray data = file.readAll();
        file.close();
        
        QDataStream stream(&data, QIODevice::ReadOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        QByteArray magic;
        magic.resize(4);
        stream.readRawData(magic.data(), 4);
        
        if (magic == "TES4")
        {
            quint16 headerSize = 0;
            quint32 updateTimeStamp = 0;
            quint32 numRecords = 0;
            stream >> headerSize >> updateTimeStamp >> numRecords;
            
            report.totalRecords = numRecords;
            
            if (numRecords == 0)
            {
                Issue issue;
                issue.message = "Empty plugin with no records";
                issue.severity = Severity::Info;
                issue.category = "Records";
                report.issues.append(issue);
            }
        }
    }
    
    return report;
}
