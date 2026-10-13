// Reports what NifParser actually loads from a .nif: node count, shape count,
// vertex/triangle totals, and whether components of node transforms are
// non-identity. Used to verify the geometry pipeline against shipped assets
// without launching the UI.
//
// Usage: nifgeomprobe <file.nif> [more.nif ...]
// Prints one line per file, then a summary. Exit code 0 any file parsed,
// 3 none parsed, 2 usage error.

#include "nifparser.hpp"
#include "logger.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <QVector>
#include <cmath>

namespace {

struct Stats {
    int nodes = 0;
    int shapes = 0;
    int verts = 0;
    int tris = 0;
    int nonIdentityTransforms = 0;
    int nonUnitScale = 0;
    bool hasNonAxisAlignedRotation = false;
    int withMaterial = 0;
    QSet<QString> materials;
};

void walk(const Nif::Node* node, Stats& s)
{
    if (!node) return;
    ++s.nodes;

    const Nif::NodeTransform& t = node->transform;
    if (t.position.x != 0.0f || t.position.y != 0.0f || t.position.z != 0.0f)
        ++s.nonIdentityTransforms;
    if (t.scale != 1.0f)
        ++s.nonUnitScale;

    const QMatrix3x3& r = t.rotation;
    const float offAxis = std::fabs(r(0,1)) + std::fabs(r(0,2))
                        + std::fabs(r(1,0)) + std::fabs(r(1,2))
                        + std::fabs(r(2,0)) + std::fabs(r(2,1));
    if (offAxis > 1e-5f)
        s.hasNonAxisAlignedRotation = true;

    for (const Nif::TriShape& shape : node->shapes) {
        ++s.shapes;
        s.verts += shape.vertices.size();
        s.tris += shape.indices.size() / 3;
        if (!shape.materialPath.isEmpty()) {
            ++s.withMaterial;
            s.materials.insert(shape.materialPath);
        }
    }
    for (const Nif::Node* child : node->children)
        walk(child, s);
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    // Without an init() the LOG_* calls in the parser stay in the pre-init
    // buffer and never surface, so this probe would report nothing about why
    // a shape is missing.
    OpenCK::Logging::Logger::instance().init(
        qEnvironmentVariable("OPENCK_LOG_DIR") + "/nifgeomprobe.log");
    const QStringList args = app.arguments();
    if (args.size() < 2) {
        QTextStream(stderr) << "usage: nifgeomprobe <file.nif> [more.nif ...]\n";
        return 2;
    }

    QTextStream out(stdout);
    int parsed = 0;

    for (int i = 1; i < args.size(); ++i) {
        const QString path = args.at(i);
        Nif::NifParser parser;
        if (!parser.load(path)) {
            out << "FAIL   " << path << "\n";
            continue;
        }
        ++parsed;
        Stats s;
        walk(parser.getRoot(), s);
        out << "nodes=" << s.nodes
            << " shapes=" << s.shapes
            << " verts=" << s.verts
            << " tris=" << s.tris
            << " nonIdentity=" << s.nonIdentityTransforms
            << " nonUnitScale=" << s.nonUnitScale
            << " nonAxisAlignedRot=" << (s.hasNonAxisAlignedRotation ? 1 : 0)
            << " withMaterial=" << s.withMaterial
            << " uniqueMaterials=" << s.materials.size()
            << "  " << path << "\n";
        if (!s.materials.isEmpty()) {
            QStringList sorted(s.materials.begin(), s.materials.end());
            for (const QString& m : sorted)
                out << "      mat " << m << "\n";
        }
    }

    out << "parsed " << parsed << " of " << (args.size() - 1) << "\n";
    return parsed == 0 ? 3 : 0;
}
