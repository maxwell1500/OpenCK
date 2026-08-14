#include "scriptrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
void ScriptRecord::load(ESMReader& esm, bool) {
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft()) {
        NAME sub = esm.readNSubHeader(); if (sub == 0) break;
        bool handled = false;
        switch (sub) {
        case 'EDID': editorId = esm.readZString(); handled = true; break;
        case 'SCHD': { qint64 sz = esm.subLeft(); scriptType = esm.readType<quint16>(); esm.skip(static_cast<int>(sz) - 2); handled = true; break; }
        case 'SCDA': esm.readRawSubData(compiledData); handled = true; break;
        case 'SCTX': scriptText = esm.readZString(); handled = true; break;
        case 'SCRO': refObjects.append(esm.readType<quint32>()); handled = true; break;
        default: break;
        }
        if (handled) continue;
        for (auto& c : components.all()) if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        if (handled) continue;
        RawSubRecord raw; raw.name = sub; esm.readRawSubData(raw.data); rawSubRecords.push_back(raw);
    }
}
void ScriptRecord::save(ESMWriter& esm) const {
    esm.writeSubZString('EDID', editorId);
    esm.startSubRecord('SCHD'); esm.writeType<quint16>(scriptType); for (int i = 2; i < 0x22; ++i) esm.writeType<quint8>(0); esm.endSubRecord();
    if (!compiledData.isEmpty()) { esm.startSubRecord('SCDA'); esm.writeRawData(compiledData.data(), compiledData.size()); esm.endSubRecord(); }
    if (!scriptText.isEmpty()) esm.writeSubZString('SCTX', scriptText);
    for (quint32 ro : refObjects) esm.writeSubData<quint32>('SCRO', ro);
    components.saveAll(esm);
    for (const auto& raw : rawSubRecords) { esm.writeRawSubRecord(raw); }
}
void ScriptRecord::blank() { editorId = ""; formId = 0; flags = 0; scriptText = ""; compiledData.clear(); scriptType = 0; refObjects.clear(); rawSubRecords.clear(); components.clear(); }
