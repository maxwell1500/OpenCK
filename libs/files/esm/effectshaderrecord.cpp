#include "effectshaderrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <cstring>

namespace {

constexpr int kDataSize = 36;  // 4 flags + 3x4 colors + 3x4 scales + 2x4 unk

} // namespace

void EfshRecord::load(ESMReader& esm, bool) {
    esm.readHeader(); formId = esm.currentFormId();
    loadOrder.clear();
    hasEdid = false;
    while (esm.isRecLeft()) {
        NAME sub = esm.readNSubHeader(); if (sub == 0) break;
        loadOrder.append(sub);
        bool handled = false;
        switch (sub) {
        case 'EDID': editorId = esm.readZString(); hasEdid = true; handled = true; break;
        case 'DATA':
        {
            QByteArray bytes;
            esm.readRawSubData(bytes);
            data.present = true;
            data.dataWidth = bytes.size();
            if (bytes.size() >= 4)
                data.shaderFlags = *reinterpret_cast<const quint32*>(bytes.constData());
            if (bytes.size() >= 16) {
                const quint8* p = reinterpret_cast<const quint8*>(bytes.constData()) + 4;
                data.fillR = p[0]; data.fillG = p[1]; data.fillB = p[2]; data.fillA = p[3];
                data.rimR = p[4];  data.rimG = p[5];  data.rimB = p[6];  data.rimA = p[7];
                data.baseR = p[8]; data.baseG = p[9]; data.baseB = p[10]; data.baseA = p[11];
            }
            if (bytes.size() >= 28) {
                const quint8* p = reinterpret_cast<const quint8*>(bytes.constData()) + 16;
                float fillScale, rimScale, baseScale;
                memcpy(&fillScale, p, 4);
                memcpy(&rimScale, p + 4, 4);
                memcpy(&baseScale, p + 8, 4);
                data.fillScale = fillScale;
                data.rimScale = rimScale;
                data.baseScale = baseScale;
            }
            if (bytes.size() >= 36) {
                const quint8* p = reinterpret_cast<const quint8*>(bytes.constData()) + 28;
                memcpy(&data.unk1, p, 4);
                memcpy(&data.unk2, p + 4, 4);
            }
            handled = true;
            break;
        }
        default: break;
        }
        if (handled) continue;
        for (auto& c : components.all()) if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        if (handled) continue;
        RawSubRecord raw; raw.name = sub; esm.readRawSubData(raw.data); rawSubRecords.push_back(raw);
    }
}
void EfshRecord::save(ESMWriter& esm) const {
    const auto writeData = [&] {
        // Prefix layout: flags[4] colors[16] scales[28] unk[36]. Emit the
        // source width (empty stays empty), growing only to fit fields an
        // edit actually touched; assembled records emit only when edited.
        int needed = 0;
        if (data.shaderFlags != 0) needed = 4;
        if (data.fillR != 255 || data.fillG != 255 || data.fillB != 255 || data.fillA != 255
            || data.rimR != 255 || data.rimG != 255 || data.rimB != 255 || data.rimA != 255
            || data.baseR != 255 || data.baseG != 255 || data.baseB != 255 || data.baseA != 255)
            needed = 16;
        if (data.fillScale != 1.0f || data.rimScale != 1.0f || data.baseScale != 1.0f)
            needed = 28;
        if (data.unk1 != 0 || data.unk2 != 0)
            needed = 36;
        const int emitSize = qMax(data.dataWidth, needed);
        if (emitSize <= 0 && data.dataWidth < 0)
        {
            // Assembled with defaults: nothing to emit (save rule).
            return;
        }
        QByteArray bytes(kDataSize, Qt::Uninitialized);
        quint8* p = reinterpret_cast<quint8*>(bytes.data());
        memcpy(p, &data.shaderFlags, 4);
        p[4] = data.fillR; p[5] = data.fillG; p[6] = data.fillB; p[7] = data.fillA;
        p[8] = data.rimR;  p[9] = data.rimG;  p[10] = data.rimB; p[11] = data.rimA;
        p[12] = data.baseR; p[13] = data.baseG; p[14] = data.baseB; p[15] = data.baseA;
        memcpy(p + 16, &data.fillScale, 4);
        memcpy(p + 20, &data.rimScale, 4);
        memcpy(p + 24, &data.baseScale, 4);
        memcpy(p + 28, &data.unk1, 4);
        memcpy(p + 32, &data.unk2, 4);
        esm.startSubRecord('DATA');
        esm.writeRawData(bytes.constData(), emitSize);
        esm.endSubRecord();
    };

    if (loadOrder.isEmpty())
    {
        if (hasEdid || !editorId.isEmpty())
            esm.writeSubZString('EDID', editorId);
        if (data.present)
            writeData();
        components.saveAll(esm);
        for (const auto& raw : rawSubRecords) { esm.writeRawSubRecord(raw); }
        return;
    }

    int rawCur = 0;
    for (NAME sub : loadOrder)
    {
        if (sub == NAME('EDID'))
        {
            esm.writeSubZString('EDID', editorId);
        }
        else if (sub == NAME('DATA'))
        {
            if (data.present)
                writeData();
        }
        else if (rawCur < rawSubRecords.size())
        {
            esm.writeRawSubRecord(rawSubRecords[rawCur++]);
        }
    }
    while (rawCur < rawSubRecords.size()) { esm.writeRawSubRecord(rawSubRecords[rawCur++]); }
}
void EfshRecord::blank() { editorId = ""; formId = 0; flags = 0; rawSubRecords.clear(); components.clear(); loadOrder.clear(); hasEdid = false; data = Data(); verbatimBody.clear(); verbatimFlags = 0; verbatimSnapshot.reset(); }
