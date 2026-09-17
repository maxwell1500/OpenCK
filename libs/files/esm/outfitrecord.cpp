#include "outfitrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "subrecordreplay.hpp"
void OutfitRecord::load(ESMReader& esm, bool) {
    esm.readHeader(); formId = esm.currentFormId();
    loadOrder.clear();
    while (esm.isRecLeft()) {
        NAME sub = esm.readNSubHeader(); if (sub == 0) break;
        loadOrder.append(sub);
        bool handled = false;
        switch (sub) {
        case 'EDID': editorId = esm.readZString(); handled = true; break;
        case 'INAM': case 'DATA':
            itemSub = sub;
            while (esm.subLeft() >= 4)
                itemFormIds.append(esm.readType<quint32>());
            if (esm.subLeft() > 0)
                esm.skip(static_cast<int>(esm.subLeft()));
            handled = true; break;
        default: break;
        }
        if (handled) continue;
        for (auto& c : components.all()) if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        if (handled) continue;
        RawSubRecord raw; raw.name = sub; esm.readRawSubData(raw.data); rawSubRecords.push_back(raw);
    }
}
void OutfitRecord::save(ESMWriter& esm) const {
    SubrecordReplay replay;
    replay.init(rawSubRecords);
    const auto writeItems = [&]() {
        esm.startSubRecord(itemSub);
        for (quint32 fid : itemFormIds) esm.writeType<quint32>(fid);
        esm.endSubRecord();
    };
    bool wroteEdid = false, wroteItems = false;
    for (NAME sub : loadOrder) {
        switch (sub) {
        case 'EDID':
            if (!wroteEdid) { esm.writeSubZString('EDID', editorId); wroteEdid = true; }
            break;
        case 'INAM': case 'DATA':
            if (!wroteItems) { writeItems(); wroteItems = true; }
            break;
        default:
            if (!components.writeSubrecord(sub, esm)) replay.write(sub, esm);
            break;
        }
    }
    if (!wroteEdid && !editorId.isEmpty()) esm.writeSubZString('EDID', editorId);
    if (!wroteItems && !itemFormIds.isEmpty()) writeItems();
    replay.writeLeftover(esm);
}
void OutfitRecord::blank() { editorId = ""; formId = 0; flags = 0; itemFormIds.clear(); itemSub = NAME('INAM'); rawSubRecords.clear(); loadOrder.clear(); components.clear(); }
