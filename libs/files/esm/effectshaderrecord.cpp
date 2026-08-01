#include "effectshaderrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <cstring>

namespace {

constexpr int kDataSize = 36;  // 4 flags + 3x4 colors + 3x4 scales + 2x4 unk

} // namespace

void EfshRecord::load(ESMReader& esm, bool) {
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft()) {
        NAME sub = esm.readNSubHeader(); if (sub == 0) break;
        bool handled = false;
        switch (sub) {
        case 'EDID': editorId = esm.readZString(); handled = true; break;
        case 'DATA':
        {
            QByteArray bytes;
            esm.readRawSubData(bytes);
            data.present = true;
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
    esm.writeSubZString('EDID', editorId);
    if (data.present) {
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
        esm.writeRawData(bytes.constData(), bytes.size());
        esm.endSubRecord();
    }
    components.saveAll(esm);
    for (const auto& raw : rawSubRecords) { esm.startSubRecord(raw.name); esm.writeRawData(raw.data.data(), raw.data.size()); esm.endSubRecord(); }
}
void EfshRecord::blank() { editorId = ""; formId = 0; flags = 0; rawSubRecords.clear(); components.clear(); data = Data(); }
