#include "papyrusproject.hpp"

#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>

#include "../../files/log/logger.hpp"

bool PapyrusProject::parse(const QString& xmlContent, PapyrusProject& out)
{
    out = PapyrusProject();

    QXmlStreamReader xml(xmlContent);
    if (xml.readNextStartElement() && xml.name() != QLatin1String("PapyrusProject"))
    {
        // Accept the root regardless of name; only fail on malformed XML.
        xml.raiseError(QStringLiteral("Unexpected root element"));
    }

    QString currentContainer; // "Imports" | "Folders" | "Scripts" | "Flags"

    while (!xml.atEnd())
    {
        const QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartElement)
        {
            const QStringView name = xml.name();
            if (name == QLatin1String("Imports")
                || name == QLatin1String("Folders")
                || name == QLatin1String("Scripts")
                || name == QLatin1String("Flags"))
            {
                currentContainer = name.toString();
            }
            else if (name == QLatin1String("Import"))
            {
                const QString text = xml.readElementText().trimmed();
                if (!text.isEmpty()) out.imports.append(text);
            }
            else if (name == QLatin1String("Folder"))
            {
                bool recurse = true;
                const QString attr = xml.attributes().value(QLatin1String("recurse")).toString();
                if (!attr.isEmpty())
                    recurse = (attr != QLatin1String("0") && attr != QLatin1String("false"));
                const QString text = xml.readElementText().trimmed();
                out.folders.append(text);
                out.folderRecurse.append(recurse);
            }
            else if (name == QLatin1String("Script"))
            {
                const QString text = xml.readElementText().trimmed();
                if (!text.isEmpty()) out.scripts.append(text);
            }
            else if (name == QLatin1String("Output"))
            {
                out.output = xml.readElementText().trimmed();
            }
            else if (name == QLatin1String("Asm"))
            {
                out.asmMode = xml.readElementText().trimmed();
            }
            else if (name == QLatin1String("Optimize"))
            {
                out.optimize = (xml.readElementText().trimmed() == QLatin1String("true"));
            }
            else if (name == QLatin1String("Release"))
            {
                out.release = (xml.readElementText().trimmed() == QLatin1String("true"));
            }
            else if (name == QLatin1String("Final"))
            {
                out.final_ = (xml.readElementText().trimmed() == QLatin1String("true"));
            }
            else if (name == QLatin1String("Flag"))
            {
                const QString text = xml.readElementText().trimmed();
                if (!text.isEmpty()) out.flags.append(text);
            }
        }
        else if (token == QXmlStreamReader::EndElement)
        {
            const QStringView name = xml.name();
            if (name == QLatin1String("Imports")
                || name == QLatin1String("Folders")
                || name == QLatin1String("Scripts")
                || name == QLatin1String("Flags"))
            {
                currentContainer.clear();
            }
        }
        else if (token == QXmlStreamReader::Invalid)
        {
            LOG_WARNING(QString("PapyrusProject::parse: XML error: %1")
                .arg(xml.errorString()));
            return false;
        }
    }

    if (xml.hasError())
    {
        LOG_WARNING(QString("PapyrusProject::parse: XML error: %1")
            .arg(xml.errorString()));
        return false;
    }
    return true;
}

bool PapyrusProject::loadFile(const QString& path, PapyrusProject& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        LOG_WARNING(QString("PapyrusProject::loadFile: cannot open %1").arg(path));
        return false;
    }
    const QString content = QString::fromUtf8(file.readAll());
    file.close();
    return parse(content, out);
}
