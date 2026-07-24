#ifndef BLENDERLAUNCHER_H
#define BLENDERLAUNCHER_H

#include <QString>
#include <QProcess>
#include <QJsonObject>
#include "../world/ckid.hpp"

/// \brief Blender integration manager for NIF file operations
/// 
/// This class provides a comprehensive interface to Blender for:
/// - Finding and validating Blender installations
/// - Exporting/importing NIF files via headless mode
/// - Generating preview images from 3D models
/// - Batch processing of model assets
/// 
/// Requirements:
/// - Blender 4.4+ with PyNifly or NifTools addon
/// - Python 3.7+ embedded in Blender
/// - NIF addon installed in Blender's addons directory
/// 
/// Usage:
/// 1. Call findBlender() to detect Blender installation
/// 2. Check isNifCompatibleVersion() for NIF support
/// 3. Use exportNifViaBlender() or importNifViaBlender() for file operations
/// 4. Use generateNifPreview() for thumbnail generation
class BlenderLauncher
{
public:
    /// \brief Information about a Blender installation
    struct BlenderInfo {
        QString path;                    ///< Path to blender.exe
        QString version;                 ///< Version string (e.g., "3.6.0")
        bool isNifCompatible;            ///< Has NIF addon installed
        bool hasPython;                  ///< Python interpreter available
        QString nifAddonName;            ///< "PyNifly", "NifTools", or empty
        QString nifAddonPath;            ///< Path to NIF addon directory
    };

    /// \brief Result of NIF export operation
    struct NifExportResult {
        bool success;                    ///< Export succeeded
        QString outputPath;              ///< Path to exported file
        QString errorMessage;            ///< Error message if failed
    };

    /// \brief Settings for preview image generation
    struct PreviewSettings {
        int width = 256;                 ///< Preview image width
        int height = 256;                ///< Preview image height
        int angles = 4;                  ///< Number of render angles (0-360)
        bool generateThumbnails = true;  ///< Generate thumbnail versions
    };

    /// \brief Find and analyze Blender installation
    /// \return BlenderInfo with path, version, and addon details
    /// 
    /// Searches common installation locations:
    /// - C:\Program Files\Blender Foundation\Blender\
    /// - Steam installation directories
    /// - Xbox Game Pass location
    static BlenderInfo findBlender();
    
    /// \brief Check if Blender is installed and available
    /// \return true if Blender found and executable
    static bool isBlenderAvailable();
    
    /// \brief Get Blender version string
    /// \param blenderPath Path to blender.exe
    /// \return Version string (e.g., "2.93", "3.6") or empty if invalid
    static QString getBlenderVersion(const QString& blenderPath);
    
    /// \brief Parse version string to comparable integer
    /// \param version Version string (e.g., "2.93")
    /// \return Comparable number (2.93 -> 293, 3.6 -> 306)
    static int parseVersionNumber(const QString& version);
    
    /// \brief Check if Blender version supports NIF operations
    /// \param version Version string to check
    /// \return true if version >= 4.4 (NIF addon compatible)
    static bool isNifCompatibleVersion(const QString& version);
    
    /// \brief Open file in Blender GUI
    /// \param filePath Path to NIF or other supported file
    /// \param blenderPath Optional specific Blender path (uses auto-detect if empty)
    /// \return true if Blender launched successfully
    static bool openInBlender(const QString& filePath, const QString& blenderPath = QString());
    
    /// \brief Export record model to temporary file and open in Blender
    /// \param modelPath Path to model file (NIF, OBJ, etc.)
    /// \param editorId Editor ID for naming temporary files
    /// \param recordType Record type enum for path resolution
    /// \return true if export successful
    static bool exportToBlender(const QString& modelPath, const QString& editorId, CkId::Type recordType);
    
    /// \brief Import modified model back from Blender output
    /// \param blenderOutputPath Path to Blender's output file
    /// \param outputPath Output parameter for final file path
    /// \return true if import successful
    static bool importFromBlender(const QString& blenderOutputPath, QString& outputPath);
    
    /// \brief Get recommended Blender path from common locations
    /// \return Path to Blender executable, or empty string if not found
    static QString getRecommendedBlenderPath();
    
    /// \brief Run Blender in headless mode with Python script
    /// \param scriptPath Path to Python script to execute
    /// \param args Additional command-line arguments
    /// \return QProcess pointer (caller responsible for cleanup)
    /// 
    /// Launches Blender with -b -P flags for background execution.
    /// Returns process immediately — check finished() signal for completion.
    static QProcess* runHeadless(const QString& scriptPath, const QStringList& args = {});
    
    /// \brief Detect PyNifly addon installation in Blender
    /// \param blenderPath Path to Blender executable
    /// \return Path to PyNifly addon directory, or empty if not found
    static QString detectPyNiflyInstallation(const QString& blenderPath);
    
    /// \brief Detect NifTools addon installation in Blender
    /// \param blenderPath Path to Blender executable
    /// \return Path to NifTools addon directory, or empty if not found
    static QString detectNifToolsInstallation(const QString& blenderPath);
    
    /// \brief Export NIF file using Blender headless mode
    /// \param nifPath Path to input NIF file
    /// \param outputPath Path for exported file (OBJ, FBX, etc.)
    /// \param blenderPath Optional Blender path (auto-detect if empty)
    /// \return NifExportResult with success status and details
    static NifExportResult exportNifViaBlender(const QString& nifPath, const QString& outputPath, const QString& blenderPath = QString());
    
    /// \brief Import NIF file using Blender headless mode
    /// \param nifPath Path to input NIF file
    /// \param blendPath Path to save as .blend file
    /// \param blenderPath Optional Blender path (auto-detect if empty)
    /// \return true if import successful
    static bool importNifViaBlender(const QString& nifPath, const QString& blendPath, const QString& blenderPath = QString());
    
    /// \brief Generate preview images for NIF file
    /// \param nifPath Path to NIF file
    /// \param previewDir Directory to save preview images
    /// \param settings Preview generation settings
    /// \return true if previews generated successfully
    static bool generateNifPreview(const QString& nifPath, const QString& previewDir, const PreviewSettings& settings = {256, 256, 4, true});
    
    /// \brief Batch export all models from plugin
    /// \param pluginPath Path to ESM/ESP plugin file
    /// \param outputDir Directory to save exported models
    /// \param blenderPath Optional Blender path (auto-detect if empty)
    /// \return true if all exports successful
    static bool batchExportModels(const QString& pluginPath, const QString& outputDir, const QString& blenderPath = QString());
    
    /// \brief Validate NIF file for common issues
    /// \param nifPath Path to NIF file
    /// \param errors Output parameter for validation error messages
    /// \return true if file is valid (no errors)
    static bool validateNifFile(const QString& nifPath, QStringList& errors);

    /// \brief Download and install the NifTools Blender addon
    /// \param blenderPath Path to Blender executable (auto-detected if empty)
    /// \return true if addon was successfully installed
    static bool installNifToolsAddon(const QString& blenderPath = QString());

    /// \brief Check if NifTools addon is installed and functional
    /// \return Installation status with details
    struct NifToolsInstallStatus {
        bool installed = false;
        bool enabled = false;
        QString version;
        QString path;
        QString errorMessage;
    };
    static NifToolsInstallStatus checkNifToolsInstallation(const QString& blenderPath = QString());

private:
    /// \brief Get list of common Blender installation paths
    static QStringList getCommonBlenderPaths();
    
    /// \brief Validate Blender installation at path
    static bool validateBlenderInstallation(const QString& path);
    
    /// \brief Find NIF addon within Blender's addons directory
    static QString findNifAddonInBlender(const QString& blenderPath, const QString& addonName);
};

#endif // BLENDERLAUNCHER_H
