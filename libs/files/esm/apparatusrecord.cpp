#include "apparatusrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
void AppaRecord::load(ESMReader& esm, bool) {
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft()) {
        NAME sub = esm.readNSubHeader(); if (sub == 0) break;
        bool handled = false;
        switch (sub) {
        case 'EDID': editorId = esm.readZString(); handled = true; break;
        case 'DATA': type = esm.readType<quint32>(); value = esm.readType<float>(); weight = esm.readType<float>(); handled = true; break;
        default: break;
        }
        if (handled) continue;
        for (auto& c : components.all()) if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        if (handled) continue;
        RawSubRecord raw; raw.name = sub; esm.readRawSubData(raw.data); rawSubRecords.push_back(raw);
    }
}
void AppaRecord::save(ESMWriter& esm) const {
    esm.writeSubZString('EDID', editorId);
    esm.startSubRecord('DATA'); esm.writeType<quint32>(type); esm.writeType<float>(value); esm.writeType<float>(weight); esm.endSubRecord();
    components.saveAll(esm);
    for (const auto& raw : rawSubRecords) { esm.writeRawSubRecord(raw); }
}
void AppaRecord::blank() { editorId = ""; formId = 0; flags = 0; type = 0; value = 0; weight = 0; rawSubRecords.clear(); components.clear(); }
