#ifndef PAPYRUSPROJECT_HPP
#define PAPYRUSPROJECT_HPP

#include <QString>
#include <QStringList>
#include <QVector>

// Parses Papyrus project files (.ppj). The real format is XML with the
// following top-level elements (per PapyrusProject.xsd):
//   <Imports>...<Import>path</Import>...</Imports>
//   <Folders>...<Folder recurse="1">path</Folder>...</Folders>
//   <Scripts>...<Script>name</Script>...</Scripts>
//   <Output>path</Output>
//   <Flags>...<Flag>name</Flag>...</Flags>
//   <Asm>None|Keep|Only|Discard</Asm>
//   <Optimize>true|false</Optimize>
//   <Release>true|false</Release>
//   <Final>true|false</Final>
struct PapyrusProject
{
    QStringList imports;
    QVector<QString> folders;          // parallel to folderRecurse
    QVector<bool> folderRecurse;
    QStringList scripts;
    QString output;
    QStringList flags;
    QString asmMode;                    // None/Keep/Only/Discard
    bool optimize = false;
    bool release = false;
    bool final_ = false;

    // Parses project XML content. Returns false on malformed XML.
    static bool parse(const QString& xmlContent, PapyrusProject& out);

    // Loads and parses the given .ppj file. Returns false if the file
    // cannot be read or is malformed.
    static bool loadFile(const QString& path, PapyrusProject& out);
};

#endif // PAPYRUSPROJECT_HPP
