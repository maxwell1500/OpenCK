#include "outfitrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
void OutfitRecord::load(ESMReader& esm, bool) {
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft()) {
        NAME sub = esm.readNSubHeader(); if (sub == 0) break;
        bool handled = false;
        switch (sub) {
        case 'EDID': editorId = esm.readZString(); handled = true; break;
        case 'INAM': case 'DATA': itemFormIds.append(esm.readType<quint32>()); handled = true; break;
        default: break;
        }
        if (handled) continue;
        for (auto& c : components.all()) if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        if (handled) continue;
        RawSubRecord raw; raw.name = sub; esm.readRawSubData(raw.data); rawSubRecords.push_back(raw);
    }
}
void OutfitRecord::save(ESMWriter& esm) const {
    esm.writeSubZString('EDID', editorId);
    for (quint32 fid : itemFormIds) esm.writeSubData<quint32>('INAM', fid);
    components.saveAll(esm);
    for (const auto& raw : rawSubRecords) { esm.writeRawSubRecord(raw); }
}
void OutfitRecord::blank() { editorId = ""; formId = 0; flags = 0; itemFormIds.clear(); rawSubRecords.clear(); components.clear(); }
