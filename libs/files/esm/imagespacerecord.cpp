#include "imagespacerecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <cstring>

namespace {

// Standard IMGS DATA layout: 48 modifier floats + 6 zone RGBA colors.
constexpr int kFloats = 48;
constexpr int kColors = 6;
constexpr int kFloatBytes = kFloats * 4;
constexpr int kColorBytes = kColors * 4;

} // namespace

void ImgsRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();

    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;

        bool handled = false;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); handled = true; break;
            case 'DATA':
            {
                QByteArray bytes;
                esm.readRawSubData(bytes);
                data.present = true;

                const int floatBytes = qMin(bytes.size(), kFloatBytes);
                if (floatBytes > 0)
                    memcpy(data.values, bytes.constData(), static_cast<size_t>(floatBytes));

                for (int i = 0; i < kColors; i++)
                {
                    const int offset = kFloatBytes + i * 4;
                    if (bytes.size() >= offset + 4)
                    {
                        const quint8* p = reinterpret_cast<const quint8*>(bytes.constData()) + offset;
                        data.color[i][0] = p[0];
                        data.color[i][1] = p[1];
                        data.color[i][2] = p[2];
                        data.color[i][3] = p[3];
                    }
                }

                if (bytes.size() > kFloatBytes + kColorBytes)
                    data.trailingBytes = bytes.mid(kFloatBytes + kColorBytes);

                handled = true;
                break;
            }
            default: break;
        }
        if (handled) continue;

        for (auto& c : components.all())
        {
            if (c->canHandle(sub))
            {
                c->handleSubrecord(sub, esm);
                handled = true;
                break;
            }
        }
        if (handled) continue;

        RawSubRecord raw;
        raw.name = sub;
        esm.readRawSubData(raw.data);
        rawSubRecords.push_back(raw);
    }
}

void ImgsRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);

    if (data.present)
    {
        QByteArray bytes(kFloatBytes + kColorBytes, Qt::Uninitialized);
        memcpy(bytes.data(), data.values, kFloatBytes);
        for (int i = 0; i < kColors; i++)
        {
            quint8* p = reinterpret_cast<quint8*>(bytes.data()) + kFloatBytes + i * 4;
            p[0] = data.color[i][0];
            p[1] = data.color[i][1];
            p[2] = data.color[i][2];
            p[3] = data.color[i][3];
        }
        bytes.append(data.trailingBytes);

        esm.startSubRecord('DATA');
        esm.writeRawData(bytes.constData(), bytes.size());
        esm.endSubRecord();
    }

    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void ImgsRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    rawSubRecords.clear();
    components.clear();
    data = Data();
}
