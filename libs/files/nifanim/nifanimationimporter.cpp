#include "nifanimationimporter.hpp"
#include "nifanimation.hpp"

#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QXmlStreamReader>
#include <QColor>

#include "logger.hpp"

NifAnimation* NifAnimationImporter::importFromJson(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("NifAnimationImporter: failed to open JSON file: %1").arg(filePath));
        return nullptr;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("NifAnimationImporter: JSON parse error: %1").arg(parseError.errorString()));
        return nullptr;
    }

    if (!doc.isObject()) {
        LOG_ERROR("NifAnimationImporter: JSON root is not an object");
        return nullptr;
    }

    QJsonObject rootObj = doc.object();

    NifAnimation* anim = new NifAnimation();
    anim->name = rootObj["name"].toString();

    QJsonArray clipsArray = rootObj["clips"].toArray();
    for (const auto& clipVal : clipsArray) {
        QJsonObject clipObj = clipVal.toObject();

        AnimClip clip;
        clip.name = clipObj["name"].toString();
        clip.duration = static_cast<float>(clipObj["duration"].toDouble());

        QJsonArray channelsArray = clipObj["channels"].toArray();
        for (const auto& chVal : channelsArray) {
            QJsonObject chObj = chVal.toObject();

            AnimChannel ch;
            ch.boneName = chObj["boneName"].toString();
            ch.type = chObj["type"].toString();
            ch.duration = static_cast<float>(chObj["duration"].toDouble());

            QJsonArray keyframesArray = chObj["keyframes"].toArray();
            for (const auto& kfVal : keyframesArray) {
                QJsonObject kfObj = kfVal.toObject();

                AnimKeyframe kf;
                kf.time = static_cast<float>(kfObj["time"].toDouble());
                kf.tx = static_cast<float>(kfObj["tx"].toDouble());
                kf.ty = static_cast<float>(kfObj["ty"].toDouble());
                kf.tz = static_cast<float>(kfObj["tz"].toDouble());
                kf.rx = static_cast<float>(kfObj["rx"].toDouble());
                kf.ry = static_cast<float>(kfObj["ry"].toDouble());
                kf.rz = static_cast<float>(kfObj["rz"].toDouble());
                kf.sx = static_cast<float>(kfObj["sx"].toDouble(1.0));
                kf.sy = static_cast<float>(kfObj["sy"].toDouble(1.0));
                kf.sz = static_cast<float>(kfObj["sz"].toDouble(1.0));
                ch.keyframes.append(kf);
            }

            clip.channels.append(ch);
        }

        anim->clips.append(clip);
    }

    if (rootObj.contains("markers")) {
        QJsonArray markersArray = rootObj["markers"].toArray();
        for (const auto& markerVal : markersArray) {
            QJsonObject markerObj = markerVal.toObject();
            AnimMarker m;
            m.time = static_cast<float>(markerObj["time"].toDouble());
            m.name = markerObj["name"].toString();
            m.color = QColor(markerObj["color"].toString());
            anim->markers.append(m);
        }
    }

    if (anim->clips.isEmpty()) {
        delete anim;
        LOG_ERROR("NifAnimationImporter: no clips found in JSON");
        return nullptr;
    }

    LOG_INFO(QString("NifAnimationImporter: imported animation from JSON: %1 (%2 clips)")
                 .arg(filePath)
                 .arg(anim->clipCount()));
    return anim;
}

NifAnimation* NifAnimationImporter::importFromXml(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QString("NifAnimationImporter: failed to open XML file: %1").arg(filePath));
        return nullptr;
    }

    QXmlStreamReader xml(&file);

    NifAnimation* anim = new NifAnimation();
    AnimClip currentClip;
    AnimChannel currentChannel;
    bool inClip = false;
    bool inChannel = false;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            if (xml.name() == u"animation") {
                anim->name = xml.attributes().value("name").toString();
            } else if (xml.name() == u"clip") {
                currentClip = AnimClip();
                currentClip.name = xml.attributes().value("name").toString();
                currentClip.duration = xml.attributes().value("duration").toFloat();
                inClip = true;
            } else if (xml.name() == u"channel" && inClip) {
                currentChannel = AnimChannel();
                currentChannel.boneName = xml.attributes().value("bone").toString();
                currentChannel.type = xml.attributes().value("type").toString();
                inChannel = true;
            } else if (xml.name() == u"keyframe" && inChannel) {
                AnimKeyframe kf;
                kf.time = xml.attributes().value("time").toFloat();
                kf.tx = xml.attributes().value("tx").toFloat();
                kf.ty = xml.attributes().value("ty").toFloat();
                kf.tz = xml.attributes().value("tz").toFloat();
                kf.rx = xml.attributes().value("rx").toFloat();
                kf.ry = xml.attributes().value("ry").toFloat();
                kf.rz = xml.attributes().value("rz").toFloat();
                if (xml.attributes().hasAttribute("sx"))
                    kf.sx = xml.attributes().value("sx").toFloat();
                if (xml.attributes().hasAttribute("sy"))
                    kf.sy = xml.attributes().value("sy").toFloat();
                if (xml.attributes().hasAttribute("sz"))
                    kf.sz = xml.attributes().value("sz").toFloat();
                currentChannel.keyframes.append(kf);
            } else if (xml.name() == u"marker") {
                AnimMarker m;
                m.time = xml.attributes().value("time").toFloat();
                m.name = xml.attributes().value("name").toString();
                m.color = QColor(xml.attributes().value("color").toString());
                anim->markers.append(m);
            }
        } else if (xml.isEndElement()) {
            if (xml.name() == u"channel" && inChannel) {
                if (!currentChannel.keyframes.isEmpty()) {
                    currentChannel.duration = currentChannel.keyframes.last().time;
                }
                currentClip.channels.append(currentChannel);
                inChannel = false;
            } else if (xml.name() == u"clip" && inClip) {
                anim->clips.append(currentClip);
                inClip = false;
            }
        }
    }

    if (xml.hasError()) {
        LOG_ERROR(QString("NifAnimationImporter: XML parse error: %1").arg(xml.errorString()));
        delete anim;
        return nullptr;
    }

    if (anim->clips.isEmpty()) {
        delete anim;
        LOG_ERROR("NifAnimationImporter: no clips found in XML");
        return nullptr;
    }

    LOG_INFO(QString("NifAnimationImporter: imported animation from XML: %1 (%2 clips)")
                 .arg(filePath)
                 .arg(anim->clipCount()));
    return anim;
}
