#include "birthsignrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
void BsgnRecord::load(ESMReader& esm, bool) {
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft()) {
        NAME sub = esm.readNSubHeader(); if (sub == 0) break;
        bool handled = false;
        switch (sub) {
        case 'EDID': editorId = esm.readZString(); handled = true; break;
        case 'TNAM': spellFormIds.append(esm.readType<quint32>()); handled = true; break;
        default: break;
        }
        if (handled) continue;
        for (auto& c : components.all()) if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        if (handled) continue;
        RawSubRecord raw; raw.name = sub; esm.readRawSubData(raw.data); rawSubRecords.push_back(raw);
    }
}
void BsgnRecord::save(ESMWriter& esm) const {
    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);
    for (quint32 sid : spellFormIds) esm.writeSubData<quint32>('TNAM', sid);
    for (const auto& raw : rawSubRecords) { esm.startSubRecord(raw.name); esm.writeRawData(raw.data.data(), raw.data.size()); esm.endSubRecord(); }
}
void BsgnRecord::blank() { editorId = ""; formId = 0; flags = 0; spellFormIds.clear(); rawSubRecords.clear(); components.clear(); }
