#include "landrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void LandRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': flags = esm.readType<quint32>(); break;
            case 'VHGT':
            {
                hasHeightData = true;
                baseHeight = esm.readType<float>();
                for (int z = 0; z < 33; ++z)
                    for (int x = 0; x < 33; ++x)
                        heightData[z][x] = esm.readType<qint8>();
                break;
            }
            case 'VNML':
            {
                hasNormalData = true;
                for (int z = 0; z < 33; ++z)
                    for (int x = 0; x < 33; ++x)
                    {
                        normalData[z][x].nx = esm.readType<qint8>();
                        normalData[z][x].ny = esm.readType<qint8>();
                        normalData[z][x].nz = esm.readType<qint8>();
                    }
                break;
            }
            case 'VCLR':
            {
                hasColorData = true;
                for (int z = 0; z < 33; ++z)
                    for (int x = 0; x < 33; ++x)
                    {
                        colorData[z][x].r = esm.readType<quint8>();
                        colorData[z][x].g = esm.readType<quint8>();
                        colorData[z][x].b = esm.readType<quint8>();
                        colorData[z][x].a = esm.readType<quint8>();
                    }
                break;
            }
            case 'VTEX':
            {
                numTextureLayers = 0;
                while (esm.isSubLeft() && numTextureLayers < 4)
                {
                    textureLayers[numTextureLayers].textureFormId = esm.readType<quint32>();
                    textureLayers[numTextureLayers].opacity = esm.readType<quint8>();
                    numTextureLayers++;
                }
                break;
            }
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

void LandRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);

    if (hasHeightData)
    {
        esm.startSubRecord('VHGT');
        esm.writeType<float>(baseHeight);
        for (int z = 0; z < 33; ++z)
            for (int x = 0; x < 33; ++x)
                esm.writeType<qint8>(heightData[z][x]);
        esm.endSubRecord();
    }

    if (hasNormalData)
    {
        esm.startSubRecord('VNML');
        for (int z = 0; z < 33; ++z)
            for (int x = 0; x < 33; ++x)
            {
                esm.writeType<qint8>(normalData[z][x].nx);
                esm.writeType<qint8>(normalData[z][x].ny);
                esm.writeType<qint8>(normalData[z][x].nz);
            }
        esm.endSubRecord();
    }

    if (hasColorData)
    {
        esm.startSubRecord('VCLR');
        for (int z = 0; z < 33; ++z)
            for (int x = 0; x < 33; ++x)
            {
                esm.writeType<quint8>(colorData[z][x].r);
                esm.writeType<quint8>(colorData[z][x].g);
                esm.writeType<quint8>(colorData[z][x].b);
                esm.writeType<quint8>(colorData[z][x].a);
            }
        esm.endSubRecord();
    }

    if (numTextureLayers > 0)
    {
        esm.startSubRecord('VTEX');
        for (int i = 0; i < numTextureLayers; ++i)
        {
            esm.writeType<quint32>(textureLayers[i].textureFormId);
            esm.writeType<quint8>(textureLayers[i].opacity);
        }
        esm.endSubRecord();
    }

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void LandRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    cellX = 0;
    cellY = 0;
    baseHeight = 0.0f;
    hasHeightData = false;
    memset(heightData, 0, sizeof(heightData));
    hasNormalData = false;
    memset(normalData, 0, sizeof(normalData));
    hasColorData = false;
    memset(colorData, 0, sizeof(colorData));
    memset(textureLayers, 0, sizeof(textureLayers));
    numTextureLayers = 0;
    rawSubRecords.clear();
}
