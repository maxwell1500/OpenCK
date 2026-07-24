#ifndef NIFPYFILEWRAPPER_H
#define NIFPYFILEWRAPPER_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QVariant>

/**
 * @brief C++ wrapper around PyNifly Python API for NIF file processing
 * 
 * This class provides a clean C++ interface to PyNifly's Python API.
 * It handles Python initialization, script execution, and data conversion.
 * 
 * PyNifly is licensed under GPL-3.0 (https://github.com/BadDogSkyrim/PyNifly)
 * OpenCK is also GPL-3.0, making this compatible.
 */
class NifPyFileWrapper
{
public:
    /**
     * @brief Structure representing a single vertex
     */
    struct Vertex {
        float x, y, z;
    };

    /**
     * @brief Structure representing a triangle (indices into vertices)
     */
    struct Triangle {
        int v0, v1, v2;
    };

    /**
     * @brief Structure representing UV coordinate
     */
    struct UV {
        float u, v;
    };

    /**
     * @brief Structure representing vertex color (RGBA)
     */
    struct Color {
        float r, g, b, a;
    };

    /**
     * @brief Structure representing mesh shape data
     */
    struct ShapeData {
        QString name;
        QVector<Vertex> vertices;
        QVector<Triangle> triangles;
        QVector<UV> uvs;
        QVector<Vertex> normals;
        QVector<Color> colors;
        QMap<QString, QString> textures; // Shader texture names -> paths
        QVector<QString> boneNames;
        bool hasSkinning;
    };

    /**
     * @brief Structure representing a NIF file's metadata
     */
    struct NifFileInfo {
        QString filePath;
        QString game; // "SKYRIM", "SKYRIMSE", "FO4", etc.
        QString rootName;
        QVector<ShapeData> shapes;
        QStringList boneNames;
        QMap<QString, QString> allTextures; // All texture references
        bool hasCollision;
        int version;
    };

    /**
     * @brief Validation error found in NIF file
     */
    struct ValidationError {
        enum Severity { Error, Warning, Info };
        
        Severity severity;
        QString field;
        QString message;
        int blockNumber;
    };

    /**
     * @brief Initialize Python interpreter and PyNifly
     * @param pyniflyPath Path to io_scene_nifly directory
     * @return true if initialization successful
     */
    static bool initialize(const QString& pyniflyPath);

    /**
     * @brief Shutdown Python interpreter
     */
    static void shutdown();

    /**
     * @brief Check if PyNifly is initialized
     */
    static bool isInitialized();

    /**
     * @brief Load a NIF file and extract all data
     * @param filePath Path to NIF file
     * @param fileInfo Output structure filled with NIF data
     * @return true if loading successful
     */
    static bool loadNif(const QString& filePath, NifFileInfo& fileInfo);

    /**
     * @brief Save a NIF file from data structures
     * @param outputPath Path to save NIF to
     * @param game Game version ("SKYRIM", "FO4", etc.)
     * @param shapes Vector of shape data to save
     * @param boneNames List of bone names for skeleton
     * @return true if saving successful
     */
    static bool saveNif(const QString& outputPath, const QString& game,
                       const QVector<ShapeData>& shapes,
                       const QVector<QString>& boneNames = QVector<QString>());

    /**
     * @brief Extract just the shape data from a NIF file
     * @param filePath Path to NIF file
     * @param shapes Output vector of shapes
     * @return true if extraction successful
     */
    static bool extractShapes(const QString& filePath, QVector<ShapeData>& shapes);

    /**
     * @brief Extract texture references from a NIF file
     * @param filePath Path to NIF file
     * @param textures Output map of texture name -> path
     * @return true if extraction successful
     */
    static bool extractTextures(const QString& filePath, QMap<QString, QString>& textures);

    /**
     * @brief Validate a NIF file for common issues
     * @param filePath Path to NIF file
     * @param errors Output vector of validation errors
     * @return true if file is valid (no errors)
     */
    static bool validateNif(const QString& filePath, QVector<ValidationError>& errors);

    /**
     * @brief Compare two NIF files and return differences
     * @param nif1 Path to first NIF
     * @param nif2 Path to second NIF
     * @param vertexChanges Output vertex differences
     * @param textureChanges Output texture differences
     * @return true if comparison successful
     */
    static bool compareNifs(const QString& nif1, const QString& nif2,
                           QVector<QString>& vertexChanges,
                           QVector<QString>& textureChanges);

    /**
     * @brief Get PyNifly version string
     */
    static QString getPyniflyVersion();

    /**
     * @brief Set the game version for new NIF files
     */
    static void setDefaultGame(const QString& game);

private:
    static bool isPythonAvailable();
    
    static QString mDefaultGame;
    static bool mInitialized;
};

#endif // NIFPYFILEWRAPPER_H
