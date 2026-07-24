#include "Treerecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void TreeRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FLAG': flags = esm.readType<quint32>(); break;
            case 'ITM2': iconPath = esm.readZString(); break;
            case 'ODIT': modelPath = esm.readZString(); break;
            case 'DATA': {
                leafCurvature = esm.readType<float>();
                leafAmplitude = esm.readType<float>();
                while (esm.isSubLeft()) { esm.readType<quint8>(); }
                break;
            }
            case 'SNAM': lodModelPath = esm.readZString(); break;
            case 'PFIG': lodFlags = esm.readType<quint32>(); break;
            default:
            {
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
                break;
            }
        }
    }
}

void TreeRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FLAG', flags);
    esm.writeSubZString('ITM2', iconPath);
    esm.writeSubZString('ODIT', modelPath);
    esm.startSubRecord('DATA');
    esm.writeType<float>(leafCurvature);
    esm.writeType<float>(leafAmplitude);
    esm.endSubRecord();
    if (!lodModelPath.isEmpty()) {
        esm.writeSubZString('SNAM', lodModelPath);
    }
    if (lodFlags != 0) {
        esm.writeSubData<quint32>('PFIG', lodFlags);
    }

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void TreeRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    iconPath = "";
    modelPath = "";
    leafCurvature = 0.0f;
    leafAmplitude = 0.0f;
    lodModelPath = "";
    lodFlags = 0;
    rawSubRecords.clear();
}
