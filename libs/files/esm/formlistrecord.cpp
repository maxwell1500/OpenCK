#include "formlistrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "subrecordreplay.hpp"
void FormListRecord::load(ESMReader& esm, bool) {
    esm.readHeader(); formId = esm.currentFormId();
    loadOrder.clear();
    while (esm.isRecLeft()) {
        NAME sub = esm.readNSubHeader(); if (sub == 0) break;
        loadOrder.append(sub);
        bool handled = false;
        switch (sub) {
        case 'EDID': editorId = esm.readZString(); handled = true; break;
        case 'LNAM': if (esm.subLeft() == 4) formIds.append(esm.readType<quint32>()); else esm.skipSub(); handled = true; break;
        default: break;
        }
        if (handled) continue;
        for (auto& c : components.all()) if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        if (handled) continue;
        RawSubRecord raw; raw.name = sub; esm.readRawSubData(raw.data); rawSubRecords.push_back(raw);
    }
}
void FormListRecord::save(ESMWriter& esm) const {
    SubrecordReplay replay;
    replay.init(rawSubRecords);
    bool wroteEdid = false;
    int lnamIdx = 0;
    for (NAME sub : loadOrder) {
        switch (sub) {
        case 'EDID':
            if (!wroteEdid) { esm.writeSubZString('EDID', editorId); wroteEdid = true; }
            break;
        case 'LNAM':
            if (lnamIdx < formIds.size()) esm.writeSubData<quint32>('LNAM', formIds[lnamIdx]);
            ++lnamIdx;
            break;
        default:
            if (!components.writeSubrecord(sub, esm)) replay.write(sub, esm);
            break;
        }
    }
    if (!wroteEdid && !editorId.isEmpty()) esm.writeSubZString('EDID', editorId);
    for (int i = lnamIdx; i < formIds.size(); ++i) esm.writeSubData<quint32>('LNAM', formIds[i]);
    replay.writeLeftover(esm);
}
void FormListRecord::blank() { editorId = ""; formId = 0; flags = 0; formIds.clear(); rawSubRecords.clear(); loadOrder.clear(); components.clear(); }
