#include "nifanimationexporter.hpp"
#include "nifanimation.hpp"

#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QXmlStreamWriter>

#include "logger.hpp"

bool NifAnimationExporter::exportToJson(const NifAnimation* animation, const QString& filePath)
{
    if (!animation) {
        LOG_ERROR("NifAnimationExporter: null animation pointer");
        return false;
    }

    QJsonObject rootObj;
    rootObj["name"] = animation->name;
    rootObj["clipCount"] = animation->clipCount();

    QJsonArray clipsArray;
    for (const auto& clip : animation->clips) {
        QJsonObject clipObj;
        clipObj["name"] = clip.name;
        clipObj["duration"] = static_cast<double>(clip.duration);
        clipObj["channelCount"] = clip.channels.size();

        QJsonArray channelsArray;
        for (const auto& ch : clip.channels) {
            QJsonObject channelObj;
            channelObj["boneName"] = ch.boneName;
            channelObj["type"] = ch.type;
            channelObj["duration"] = static_cast<double>(ch.duration);
            channelObj["keyframeCount"] = ch.keyframes.size();

            QJsonArray keyframesArray;
            for (const auto& kf : ch.keyframes) {
                QJsonObject kfObj;
                kfObj["time"] = static_cast<double>(kf.time);
                kfObj["tx"] = static_cast<double>(kf.tx);
                kfObj["ty"] = static_cast<double>(kf.ty);
                kfObj["tz"] = static_cast<double>(kf.tz);
                kfObj["rx"] = static_cast<double>(kf.rx);
                kfObj["ry"] = static_cast<double>(kf.ry);
                kfObj["rz"] = static_cast<double>(kf.rz);
                kfObj["sx"] = static_cast<double>(kf.sx);
                kfObj["sy"] = static_cast<double>(kf.sy);
                kfObj["sz"] = static_cast<double>(kf.sz);
                keyframesArray.append(kfObj);
            }
            channelObj["keyframes"] = keyframesArray;
            channelsArray.append(channelObj);
        }
        clipObj["channels"] = channelsArray;
        clipsArray.append(clipObj);
    }
    rootObj["clips"] = clipsArray;

    QJsonArray markersArray;
    for (const auto& marker : animation->markers) {
        QJsonObject markerObj;
        markerObj["time"] = static_cast<double>(marker.time);
        markerObj["name"] = marker.name;
        markerObj["color"] = marker.color.name();
        markersArray.append(markerObj);
    }
    rootObj["markers"] = markersArray;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("NifAnimationExporter: failed to open file for writing: %1").arg(filePath));
        return false;
    }

    QJsonDocument doc(rootObj);
    if (!file.write(doc.toJson(QJsonDocument::Indented))) {
        LOG_ERROR(QString("NifAnimationExporter: failed to write JSON data to: %1").arg(filePath));
        return false;
    }
    file.close();

    LOG_INFO(QString("NifAnimationExporter: exported animation to JSON: %1").arg(filePath));
    return true;
}

bool NifAnimationExporter::exportToXml(const NifAnimation* animation, const QString& filePath)
{
    if (!animation) {
        LOG_ERROR("NifAnimationExporter: null animation pointer");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR(QString("NifAnimationExporter: failed to open file for writing: %1").arg(filePath));
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);

    xml.writeStartDocument();
    xml.writeStartElement("animation");
    xml.writeAttribute("name", animation->name);
    xml.writeAttribute("clipCount", QString::number(animation->clipCount()));

    for (const auto& clip : animation->clips) {
        xml.writeStartElement("clip");
        xml.writeAttribute("name", clip.name);
        xml.writeAttribute("duration", QString::number(static_cast<double>(clip.duration)));

        for (const auto& ch : clip.channels) {
            xml.writeStartElement("channel");
            xml.writeAttribute("bone", ch.boneName);
            xml.writeAttribute("type", ch.type);

            for (const auto& kf : ch.keyframes) {
                xml.writeStartElement("keyframe");
                xml.writeAttribute("time", QString::number(static_cast<double>(kf.time)));
                xml.writeAttribute("tx", QString::number(static_cast<double>(kf.tx)));
                xml.writeAttribute("ty", QString::number(static_cast<double>(kf.ty)));
                xml.writeAttribute("tz", QString::number(static_cast<double>(kf.tz)));
                xml.writeAttribute("rx", QString::number(static_cast<double>(kf.rx)));
                xml.writeAttribute("ry", QString::number(static_cast<double>(kf.ry)));
                xml.writeAttribute("rz", QString::number(static_cast<double>(kf.rz)));
                xml.writeAttribute("sx", QString::number(static_cast<double>(kf.sx)));
                xml.writeAttribute("sy", QString::number(static_cast<double>(kf.sy)));
                xml.writeAttribute("sz", QString::number(static_cast<double>(kf.sz)));
                xml.writeEndElement(); // keyframe
            }

            xml.writeEndElement(); // channel
        }

        xml.writeEndElement(); // clip
    }

    xml.writeStartElement("markers");
    for (const auto& marker : animation->markers) {
        xml.writeStartElement("marker");
        xml.writeAttribute("time", QString::number(static_cast<double>(marker.time)));
        xml.writeAttribute("name", marker.name);
        xml.writeAttribute("color", marker.color.name());
        xml.writeEndElement();
    }
    xml.writeEndElement(); // markers

    xml.writeEndElement(); // animation
    xml.writeEndDocument();

    file.close();

    LOG_INFO(QString("NifAnimationExporter: exported animation to XML: %1").arg(filePath));
    return true;
}
