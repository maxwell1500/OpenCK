#ifndef AUDIOPIPELINETOOLS_H
#define AUDIOPIPELINETOOLS_H

#include <QString>
#include <QStringList>
#include <QVector>

// AudioPipelineTools wraps the external audio-generation tools the real
// Creation Kit integrates: LipGenerator (Fonix phoneme analysis -> .lip),
// the FaceFX compiler (ffxc + .facefx actors), the Wwise soundbank builder,
// and RoboVoicer TTS. OpenCK finds each tool, validates its install, and
// builds the command lines that produce lip files / banks / voice-overs.
class AudioPipelineTools
{
public:
    enum class Tool
    {
        LipGenerator,
        FaceFx,
        Wwise,
        RoboVoicer
    };

    static QString toolName(Tool tool);

    // Searches the standard Tools\ directory layout for an executable and
    // returns its full path, or empty when missing.
    static QString findTool(Tool tool, const QString& toolsDir);

    // The phoneme-analysis data file LipGenerator needs next to its exe.
    static QString lipDataFile(const QString& toolsDir);

    // Builds the LipGenerator command-line: input WAV -> output .lip with
    // the given phoneme data file and optional sampling rate.
    static QStringList lipGeneratorArguments(const QString& lipGeneratorPath,
                                             const QString& wavPath,
                                             const QString& lipPath,
                                             const QString& dataFile,
                                             int sampleRate = 22050);

    // Builds the FaceFX compiler command line.
    static QStringList facefxArguments(const QString& ffxcPath,
                                       const QString& actorProject,
                                       const QString& outputDir);

    // Builds the RoboVoicer command line (TTS text -> WAV).
    static QStringList roboVoicerArguments(const QString& roboVoicerPath,
                                           const QString& text,
                                           const QString& outputWavPath);

    // The default Wwise external codec id used by the Creation Kit.
    static int wwiseExternalCodecId();
};

#endif // AUDIOPIPELINETOOLS_H
