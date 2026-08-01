#include "navmrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
void NavmRecord::load(ESMReader& esm, bool) {
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft()) {
        NAME sub = esm.readNSubHeader(); if (sub == 0) break;
        bool handled = false;
        switch (sub) {
        case 'EDID': editorId = esm.readZString(); handled = true; break;
        case 'NVMV': {
            qint64 count = esm.subLeft() / 12;
            for (qint64 i = 0; i < count; ++i) {
                float x = esm.readType<float>(); float y = esm.readType<float>(); float z = esm.readType<float>();
                vertices.append(QVector3D(x, y, z));
            }
            handled = true; break;
        }
        case 'NVTR': {
            qint64 count = esm.subLeft() / 7;
            for (qint64 i = 0; i < count; ++i) {
                NavmTriangle tri;
                tri.v0 = esm.readType<qint16>(); tri.v1 = esm.readType<qint16>(); tri.v2 = esm.readType<qint16>();
                tri.flags = esm.readType<quint8>();
                triangles.append(tri);
            }
            handled = true; break;
        }
        case 'NVCA': {
            qint64 count = esm.subLeft() / 6;
            for (qint64 i = 0; i < count && i < triangles.size(); ++i) {
                triangles[i].edge0 = esm.readType<qint16>();
                triangles[i].edge1 = esm.readType<qint16>();
                triangles[i].edge2 = esm.readType<qint16>();
            }
            handled = true; break;
        }
        case 'NVDP': {
            qint64 count = esm.subLeft() / 4;
            for (qint64 i = 0; i < count; ++i) externalConnections.append(esm.readType<quint32>());
            handled = true; break;
        }
        case 'NVMI': cellFormId = esm.readType<quint32>(); handled = true; break;
        default: break;
        }
        if (handled) continue;
        for (auto& c : components.all()) if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        if (handled) continue;
        RawSubRecord raw; raw.name = sub; esm.readRawSubData(raw.data); rawSubRecords.push_back(raw);
    }
}
void NavmRecord::save(ESMWriter& esm) const {
    esm.writeSubZString('EDID', editorId);
    if (!vertices.isEmpty()) {
        esm.startSubRecord('NVMV');
        for (const auto& v : vertices) { esm.writeType<float>(v.x()); esm.writeType<float>(v.y()); esm.writeType<float>(v.z()); }
        esm.endSubRecord();
    }
    if (!triangles.isEmpty()) {
        esm.startSubRecord('NVTR');
        for (const auto& tri : triangles) { esm.writeType<qint16>(tri.v0); esm.writeType<qint16>(tri.v1); esm.writeType<qint16>(tri.v2); esm.writeType<quint8>(tri.flags & 0xFF); }
        esm.endSubRecord();
        esm.startSubRecord('NVCA');
        for (const auto& tri : triangles) { esm.writeType<qint16>(tri.edge0); esm.writeType<qint16>(tri.edge1); esm.writeType<qint16>(tri.edge2); }
        esm.endSubRecord();
    }
    if (!externalConnections.isEmpty()) {
        esm.startSubRecord('NVDP');
        for (quint32 conn : externalConnections) esm.writeType<quint32>(conn);
        esm.endSubRecord();
    }
    if (cellFormId != 0) esm.writeSubData<quint32>('NVMI', cellFormId);
    components.saveAll(esm);
    for (const auto& raw : rawSubRecords) { esm.startSubRecord(raw.name); esm.writeRawData(raw.data.data(), raw.data.size()); esm.endSubRecord(); }
}
void NavmRecord::blank() { editorId = ""; formId = 0; flags = 0; cellFormId = 0; vertices.clear(); triangles.clear(); externalConnections.clear(); rawSubRecords.clear(); components.clear(); }
