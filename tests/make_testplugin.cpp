// Helper that writes a minimal test plugin (not a QTest). Used by manual
// CLI verification and by the CLI integration test harness.
#include <QFile>
#include <QString>
#include <cstdio>

#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/esm/npcrecord.hpp"

int main(int argc, char* argv[])
{
    const QString path = (argc > 1)
        ? QString::fromLocal8Bit(argv[1])
        : QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/clitest.esp");

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        printf("open fail: %s\n", path.toUtf8().constData());
        return 1;
    }

    ESMWriter writer;
    writer.setAuthor("CLI Test");
    writer.save(file);

    NpcRecord npc;
    npc.editorId = "CLITestNPC";
    npc.formId = 0x1234;
    npc.fullName = "CLI Test Character";
    npc.level = 5;

    RecHeader recHeader;
    recHeader.id = 0x1234;
    writer.startRecord('NPC_', recHeader);
    npc.save(writer);
    writer.endRecord();

    writer.close();
    file.close();
    printf("wrote %s (%lld bytes)\n", path.toUtf8().constData(),
        static_cast<long long>(QFile(path).size()));
    return 0;
}
