#include "constructibleobjectrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
void CobjRecord::load(ESMReader& esm, bool) {
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft()) {
        NAME sub = esm.readNSubHeader(); if (sub == 0) break;
        bool handled = false;
        switch (sub) { case 'EDID': editorId = esm.readZString(); handled = true; break; default: break; }
        if (handled) continue;
        for (auto& c : components.all()) if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        if (handled) continue;
        RawSubRecord raw; raw.name = sub; esm.readRawSubData(raw.data); rawSubRecords.push_back(raw);
    }
}
void CobjRecord::save(ESMWriter& esm) const {
    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);
    for (const auto& raw : rawSubRecords) { esm.writeRawSubRecord(raw); }
}
void CobjRecord::blank() { editorId = ""; formId = 0; flags = 0; rawSubRecords.clear(); components.clear(); }

quint32 CobjRecord::createdObjectId() const
{
    for (const RawSubRecord& raw : rawSubRecords)
    {
        if (raw.name != NAME('CNAM') || raw.data.size() < 4)
            continue;
        const uchar* p = reinterpret_cast<const uchar*>(raw.data.constData());
        return static_cast<quint32>(p[0])
            | (static_cast<quint32>(p[1]) << 8)
            | (static_cast<quint32>(p[2]) << 16)
            | (static_cast<quint32>(p[3]) << 24);
    }
    return 0;
}
